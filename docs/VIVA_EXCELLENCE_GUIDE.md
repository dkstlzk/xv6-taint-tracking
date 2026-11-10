# Viva Voce & Demonstration Excellence Guide
## IIT Jodhpur — CSL3030 Operating Systems Lab — Project 3 (xv6-riscv)

This guide provides the core design points, code landmarks, and examiner defense strategies to ensure a top-tier project evaluation.

---

## 1. Quick Technical Reference & Code Landmarks

| Requirement & Subsystem | File & Function | Key Logic & Implementation Details |
| :--- | :--- | :--- |
| **Inode Persistence (Option C)** | `kernel/fs.h`<br>`kernel/fs.c` | Repurposed 1 direct pointer for `uint untrusted` (`NDIRECT 11`, 64B dinode). Checked via `_Static_assert(sizeof(struct dinode) == 64)`. Synchronous persistence via `iupdate(ip)`. Clean reallocation zeroing in `ialloc()`. |
| **Process Taint Lifecycle** | `kernel/proc.h`<br>`kernel/proc.c` | `int tainted` in `struct proc`. Initialized to `0` in `allocproc` & `freeproc`. Inherited across `kfork()` under child's `p->lock`. Preserved across `kexec()`. |
| **Taint Latching (Read Gate)** | `kernel/file.c`<br>`fileread()` | Positive-byte read gate: `if ((r = readi(f->ip, 1, addr, f->off, n)) > 0) { if (f->ip->untrusted) myproc()->tainted = 1; }`. Zero-byte and failed reads do not taint. |
| **Write-Down Prevention Gate** | `kernel/file.c`<br>`kernel/sysfile.c` | Tainted process writing to clean regular file is denied: `if (myproc()->tainted && f->ip->type == T_FILE && !f->ip->untrusted) return -1;`. `sys_open` intercepts `O_WRONLY`, `O_RDWR`, `O_TRUNC` before truncation. `create()` sets `ip->untrusted = 1` (born untrusted). |
| **Directory & Exec Containment** | `kernel/taint.c`<br>`kernel/exec.c` | `in_safe(ip)` checks directory inode membership of `/safe` (inode 3). Rejects `.`, `..`, root `/`, and `/safe` self-match. Pre-lock directory check in `kexec` avoids deadlocks; post-lock check `ip_untrusted(ip)` prevents untrusted binary execution inside `/safe`. |
| **Pipe Concurrency Watermark** | `kernel/pipe.c`<br>`kernel/pipe.h` | Monotonic high-watermark `tainted_hi` & `has_taint`. Incremented per-byte on tainted write. Signed distance `(int)(pi->tainted_hi - pi->nread) > 0` latches taint on reader. Multi-core safe (`CPUS=3`). |
| **Canonical Denial Logs** | Kernel Hooks | Exactly 4 canonical log formats emitted once per refusal: `taint: unlink refused for pid %d\n`, `taint: link refused...`, `taint: write refused...`, `taint: exec refused...`. |
| **Host Agent & QEMU Transport** | `agent_lab/xv6agent/` | `validate_action()` schema allowlist. `QemuBridge` with random sentinel framing `@@DONE<nonce>@@`, negative lookbehind `(?<!echo )`, buffer size bounding (80 chars), and crash recovery. |
| **Live LLM Integration** | `agent_lab/xv6agent/planner.py` | Pluggable `ModelPlanner` supporting Gemini, OpenAI, and Ollama with graceful fallback to `DeterministicPlanner`. |

---

## 2. Examiners' Hard Questions & Bulletproof Defenses

### Q1: Why didn't you just add a new 4-byte or 2-byte field to `struct dinode`?
> **Answer**: `mkfs.c` strictly asserts that block size (`BSIZE = 1024`) is evenly divisible by `sizeof(struct dinode)`: `1024 % sizeof(struct dinode) == 0`. Adding even a `short` increases `dinode` from 64 to 68 bytes ($1024 \pmod{68} \neq 0$), breaking the block geometry and crashing `mkfs`. We chose **Option C**: reducing `NDIRECT` from 12 to 11 and placing `uint untrusted` in the freed 4 bytes. This guarantees `sizeof(struct dinode) == 64` and `IPB == 16`, preserving disk layout and filesystem compatibility.

### Q2: Why track pipe taint in `struct pipe` rather than `struct file`?
> **Answer**: In UNIX, `dup()` and `fork()` create multiple descriptors sharing the same `struct file`, while the write-end and read-end of a pipe are two distinct `struct file` instances. Storing taint on `struct file` fails because the read-end would never see the mark placed by the write-end. Taint must be tracked on the shared IPC channel buffer itself: `struct pipe`.

### Q3: What is the watermark algorithm in `pipe.c` and does it have race conditions?
> **Answer**: We track `tainted_hi` (the cumulative byte index of the most recently written untrusted byte) and `has_taint`. On write, `tainted_hi` is updated per-byte under `pi->lock`. A reader latches taint if `(int)(pi->tainted_hi - pi->nread) > 0`. Because pipe buffer capacity is 512 bytes ($\ll 2^{31}$), signed 32-bit modular arithmetic completely avoids circular buffer wraparound bugs. Across 320 multicore trials (`CPUS=3`), zero false negatives were observed.

### Q4: How does `/safe` containment prevent path traversal and alias attacks?
> **Answer**: We verify containment by **inode entry identity**, not naive string prefix matching. `in_safe(ip)` scans the directory entries of `/safe` (inode 3) to confirm the inode is a direct child. This makes it immune to `..`, symlinks, or current working directory tricks. Furthermore, `kexec` inspects `!ip_untrusted(ip)` after loading under lock, preventing attackers from copying untrusted binaries into `/safe` (Attack A16).

### Q5: What is the difference between Layer-1 and Layer-2 defense?
> **Answer**:
> - **Layer 1 (Host Tool Validation)**: Catches prompt-injected tool calls (e.g., `delete_file`, malformed paths) before commands reach the guest console.
> - **Layer 2 (Guest Kernel Policy)**: Catches malicious execution inside the guest (e.g. `cat inbox_cmd.txt | sh`), where a process directly invokes system calls bypassing the agent. The kernel enforces mandatory information flow at the syscall boundary.

---

## 3. Five-Minute Live Demonstration Runbook

To demonstrate the full project live in under 5 minutes:

```bash
# 1. Run all 39 host unit and integration tests (takes 10 seconds)
python3 -m unittest discover -s agent_lab/tests

# 2. Run the 4 guest kernel test suites inside QEMU (takes 30 seconds)
python3 -m scripts.run_suite ttest --cpus 1 --log /tmp/ttest.log
python3 -m scripts.run_suite ftest --cpus 1 --log /tmp/ftest.log
python3 -m scripts.run_suite ptest --cpus 1 --log /tmp/ptest.log
python3 -m scripts.run_suite atest --cpus 1 --log /tmp/atest.log

# 3. Demonstrate Layer-1 Prompt Injection Interception
python3 -m agent_lab.demo_agent

# 4. Demonstrate Layer-2 Kernel Defense (Runs A, B, C)
python3 -m agent_lab.demo_kernel --baseline-root /tmp/xv6-stock

# 5. Demonstrate Live LLM Integration (Optionally with GEMINI_API_KEY)
python3 -m agent_lab.demo_llm

# 6. Verify Submission Integrity
bash scripts/verify_individual.sh
```
