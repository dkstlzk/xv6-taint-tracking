# Two-Layer Taint Tracking & Agent Containment on xv6-riscv

**Authors**: Operating Systems Lab Team (CSL3030 Project 3)  
**Target Architecture**: 64-bit RISC-V (QEMU `virt` platform, `sbi` interface, multicore `CPUS=3`)  
**Pinned Source Revision**: `f780d7850765d74cbdcd1fff4b822d8829f7c9bd` (MIT xv6-riscv)  

---

## 1. Threat Model & Two-Layer Defense Architecture

Autonomous AI agents executing operating system commands via Large Language Models present critical security vulnerabilities: prompt injection in untrusted files can induce the model to emit destructive system commands (`rm`, `truncate`, arbitrary binary execution). To counteract this without relying solely on fallible LLM self-policing, we designed and implemented a **Two-Layer Defense Architecture**:

```
                       [ USER / EXTERNAL INPUT ]
                                   │
                                   ▼
                   ┌───────────────────────────────┐
                   │    Layer 1: Host Agent Lab    │
                   │  - Parameter & Schema Check   │
                   │  - Read-only Tool Enforcement │
                   │  - Gullible Injection Defense │
                   └───────────────┬───────────────┘
                                   │ (Commands via QEMU Bridge)
                                   ▼
                   ┌───────────────────────────────┐
                   │  Layer 2: Guest Kernel Hooks  │
                   │  - Process Taint Latch (I1)   │
                   │  - Disk Inode Persistence(I3) │
                   │  - Watermark Pipe Tracking(I5)│
                   │  - Write-down & Exec Gates(I2)│
                   └───────────────────────────────┘
```

1. **Layer 1 (Host-Side Agent Filtering)**: The Python dispatcher validates tool invocation parameters using strict regular expressions and whitelist schemas (`validate_action`). Injection attempts are intercepted before reaching the virtual machine.
2. **Layer 2 (Hardware/OS Kernel Enforcement)**: Even if Layer 1 is completely bypassed (simulated by a gullible planner or direct adversarial shell access), the modified xv6 kernel enforces information-flow taint tracking at the hardware/syscall boundary. Untrusted input irrevocably taints the calling process, forbidding write-downs to clean files or unauthorized binary execution.

---

## 2. Kernel Taint Tracking & Formal Invariants

The kernel guarantees five core security invariants across all execution paths:

### Invariant I1: Irreversible Process-Granularity Taint & Partial-Copy Tracking
Each process control block (`struct proc`) carries an integer flag `p->tainted`. Reading $k > 0$ bytes from an untrusted inode into user memory unconditionally transitions the caller to tainted:
$$\text{Copied}(ip, k) \land (ip.\text{untrusted} = 1) \land (k > 0) \implies p.\text{tainted} \leftarrow 1$$
Once set, $p.\text{tainted}$ is monotonically non-decreasing over the lifetime of the process. In `kfork()`, child processes inherit the exact taint state of the parent under `p->lock`.

**Discovered Partial-Read Taint Bug & Fix**:
In the initial implementation, `fileread()` checked only the final return value `r > 0` from `readi()`. However, `copyout()` in `kernel/vm.c` iterates page-by-page (`PGSIZE = 4096`) and `readi()` in `kernel/fs.c` iterates block-by-block (`BSIZE = 1024`). When a `read()` targeting an untrusted file succeeded on an earlier page or disk block and then faulted on a subsequent unmapped/read-only boundary, `readi()` overwrote `tot = -1` and returned `-1` even though untrusted bytes had already reached user memory—leaving the caller untainted (`p->tainted == 0`).
To close this gap:
- `copyout_partial()` (`kernel/vm.c`), `either_copyout_partial()` (`kernel/proc.c`), and `readi_partial()` (`kernel/fs.c`) track the exact byte count transferred into user memory via an output `copied` counter regardless of whether a later page or block faults and forces `-1`.
- `fileread()` (`kernel/file.c`) latches `myproc()->tainted = 1` (and emits `AUDIT_PROCESS_TAINTED`) whenever `copied > 0` on an untrusted inode—even if `read()` returns `-1`—while reads transferring zero bytes (`n == 0`, EOF, or immediate invalid-pointer failures with `copied == 0`) leave a clean process clean.

### Invariant I2: Strict Write-Down Prevention
A tainted process cannot mutate any clean regular disk file (`ip->type == T_FILE \land ip.\text{untrusted} == 0`). Enforcement intercepts write attempts at three distinct layers:
- `filewrite()`: Denies writing untrusted data to clean files, returning $-1$ and logging `TAINT_REFUSE_WRITE` (`"taint: write refused for pid %d\n"`).
- `sys_open()`: Denies `O_WRONLY`, `O_RDWR`, or `O_TRUNC` flags on clean files by tainted callers *before* `itrunc()` can empty existing blocks.
- `sys_unlink()` & `sys_link()`: Denies directory unlinking and hard-link creation, preventing name laundering.

### Invariant I3: Inode Persistence, Reduced Max File Size & Disk Image Rebuild
On-disk `struct dinode` repopulates one direct block address slot to store `uint untrusted`. Under the 64-byte Option C format (`NDIRECT 11` instead of stock `12`, plus `1` indirect block), inode size and block alignment remain untouched ($\text{sizeof}(\text{struct dinode}) = 64\text{ bytes}$, $\text{IPB} = 16$).
- **Reduced Maximum File Size**: `MAXFILE` drops from stock `268` blocks (`12 + 256 = 274,432` bytes) to `267` blocks (`11 + 256 = 273,408` bytes), a reduction of exactly `1` block (`1024` bytes).
- **Disk Image Incompatibility**: Because the on-disk `struct dinode` layout changed (`NDIRECT 11` and `untrusted` field), pre-existing or stock xv6 `fs.img` disk images are incompatible and **must be rebuilt** (`make clean && make fs.img`) via `mkfs/mkfs`.
- **Lifecycle & Crash Safety**: Any newly created regular file by a tainted process is marked born-untrusted ($ip.\text{untrusted} \leftarrow 1$) during `create()`/`ialloc()`. Taint is committed durably to the disk log via `iupdate(ip)`. When an unlinked inode is freed, its fields are cleared, preventing phantom taint on reallocation (confirmed via T13).

---

## 3. Pipe Concurrency & Watermark Mathematics (Member D)

In a UNIX pipe, inter-process communication allows concurrent reads and writes. To prevent clean processes from consuming untrusted bytes without becoming tainted—while avoiding complex per-byte linked lists—we implemented a monotonic high-watermark tracking mechanism.

### Watermark Tracking Algorithm
Let $n_{\text{write}}$ be the cumulative bytes written to the pipe, $n_{\text{read}}$ the cumulative bytes read, and $H_{\text{taint}}$ the high watermark offset (`tainted_hi` in `struct pipe`):
1. **On Tainted Write**: For a tainted process writing $m$ bytes:
   $$H_{\text{taint}} \leftarrow n_{\text{write}} + m$$
2. **On Clean Write**: For a clean process writing $m$ bytes:
   $$H_{\text{taint}} \leftarrow \max(H_{\text{taint}}, \text{unchanged})$$
3. **On Read**: A reader reading $r > 0$ bytes checks the signed offset difference $\Delta$:
   $$\Delta = (H_{\text{taint}} - n_{\text{read}}) \pmod{2^{32}}$$
   If $\text{signed}(\Delta) > 0$, the byte range being read contains or was contaminated by untrusted input, and the reader process transitions to $p.\text{tainted} \leftarrow 1$.

### Concurrency Safety Proof
Because $n_{\text{write}} - n_{\text{read}} \le \text{PIPESIZE} = 512 \ll 2^{31}$, signed 32-bit modular subtraction eliminates integer wrap-around vulnerabilities during active communication. Any tainted byte entered into the buffer guarantees that $\Delta > 0$ for all readers until that byte is consumed.

**Conservative Trade-off**: Because tainting tracks a single monotonic high-watermark (`tainted_hi`) rather than per-byte labels, if clean bytes sit ahead of an unread tainted byte in the FIFO buffer (e.g., a clean writer writes before a tainted writer, or both interleave), a reader reading those clean bytes satisfies $(H_{\text{taint}} - n_{\text{read}}) > 0$ and latches tainted early. Conversely, zero-byte pipe reads and immediate failed `copyout` transfers (`i <= 0`) leave clean readers clean. This intentional conservative over-approximation preserves safety and eliminates lock/race hazards under multicore execution (`CPUS=3`).

---

## 4. Path-Level Execution Containment (`TAINT_SAFE_DIR` = `/safe`)

Tainted processes are forbidden from executing arbitrary binaries outside the centralized safe directory (`TAINT_SAFE_DIR` in `kernel/taint.h`, default `"/safe"`):
- `in_safe(struct inode *ip)` dynamically resolves the `/safe` directory inode via `namei(TAINT_SAFE_DIR)` (not a hardcoded inode number or string path prefix) and scans its directory entries to confirm `ip->inum` and `ip->dev` match a direct child entry. Special directory entries (`.`, `..`), root (`/`), subdirectories inside `/safe`, and `/safe` itself are disallowed. Hard links inside `/safe` resolve via their target `inum`.
- **Pre-lock & Post-lock Verification**:
  - `exec.c` verifies `/safe` containment before acquiring inode sleeplocks, eliminating lock inversion deadlocks.
  - After acquiring `ilock(ip)`, the kernel validates that the target is a regular file and clean (`ip->type == T_FILE && !ip_untrusted(ip)`), preventing attackers from copying or creating an untrusted ELF binary inside `/safe` and executing it (OH3/A16).

---

## 5. Configuration Coupling & Verification

- **Centralized & Parameterized Settings**: Policy path (`TAINT_SAFE_DIR`) and canonical kernel refusal templates (`TAINT_REFUSE_*`) live in `kernel/taint.h` (mirrored by `SAFE_DIR` in `agent_lab/xv6agent/config.py`), user test constants live as `#define` macros in `user/*.c`, and `scripts/stress_campaign.py` accepts `--cpus` and `--timeout`. Coupled hardware/layout/buffer limits (RAM `-m`/`PHYSTOP`, `struct dinode`/`FSSIZE`/`fs.img`, `USERSTACK`, and `PIPESIZE`) are documented in [LIMITS.md](../LIMITS.md).
- **Experimental Verification**:
  - Guest verification (`ttest`, `ftest`, `ptest`, `atest`, `audtest`) passes across both `CPUS=1` and `CPUS=3`, including `T3b`–`T3d` covering zero-byte reads, non-copying invalid pointers, and partial copy failures across page and filesystem-block boundaries.
  - Full host/bridge Python suite passes (`39/39` tests) alongside Gate G1–G7b campaigns, stock `usertests` regressions, multi-boot persistence checks, and `CPUS=3` concurrency stress trials.

