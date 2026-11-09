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

### Invariant I1: Irreversible Process-Granularity Taint
Each process control block (`struct proc`) carries an integer flag `p->tainted`. Reading $k > 0$ bytes from an untrusted inode unconditionally transitions the caller to tainted:
$$\text{Read}(ip, k) \land (ip.\text{untrusted} = 1) \land (k > 0) \implies p.\text{tainted} \leftarrow 1$$
Once set, $p.\text{tainted}$ is monotonically non-decreasing over the lifetime of the process. In `kfork()`, child processes inherit the exact taint state of the parent under `p->lock`.

### Invariant I2: Strict Write-Down Prevention
A tainted process cannot mutate any clean regular disk file (`ip->type == T_FILE \land ip.\text{untrusted} == 0`). Enforcement intercepts write attempts at three distinct layers:
- `filewrite()`: Denies writing untrusted data to clean files, returning $-1$ and logging `taint: write refused for pid %d\n`.
- `sys_open()`: Denies `O_WRONLY`, `O_RDWR`, or `O_TRUNC` flags on clean files by tainted callers *before* `itrunc()` can empty existing blocks.
- `sys_unlink()` & `sys_link()`: Denies directory unlinking and hard-link creation, preventing name laundering.

### Invariant I3: Inode Persistence & Born-Untrusted Lifecycle
On-disk `struct dinode` repopulates one unused block address to store `uint untrusted`. Under the 64-byte Option C format (`NDIRECT 11`, 1 indirect block), inode size and block alignment remain untouched ($\text{sizeof}(\text{struct dinode}) = 64\text{ bytes}$, $\text{IPB} = 16$). Any newly created regular file by a tainted process is marked born-untrusted ($ip.\text{untrusted} \leftarrow 1$) during `ialloc()`. Taint is committed durably to the disk log via `iupdate(ip)`. When an unlinked inode is freed, its fields are cleared, preventing phantom taint on reallocation (confirmed via T13).

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

**Conservative Trade-off**: If a clean writer inserts bytes *before* an unread tainted byte, a reader reading those clean bytes will latch tainted early. This intentional false-positive preserves system safety and avoids race conditions under multicore execution (`CPUS=3`).

---

## 4. Path-Level Execution Containment (`/safe`)

Tainted processes are forbidden from executing arbitrary binaries outside the designated `/safe` directory:
- `in_safe(struct inode *ip)` resolves the parent directory inode of `/safe` (inode 3) and verifies direct child membership via directory scanning. Special entries (`.`, `..`), root (`/`), and `/safe` itself are disallowed.
- **Pre-lock & Post-lock Verification**:
  - `exec.c` verifies `/safe` containment before acquiring inode sleeplocks, eliminating lock inversion deadlocks.
  - After loading the ELF header under lock, the kernel validates that the binary itself is clean (`!ip_untrusted(ip)`), preventing attackers from copying an untrusted binary into `/safe` and executing it (OH3).

---

## 5. Experimental Verification & Release Evidence

The implementation was systematically verified against 53 formal requirement rows across 7 gates:
- **Clean Regressions**: 3 consecutive clean-build runs on both `CPUS=1` and `CPUS=3` with 0 failures and 0 inconclusive outcomes.
- **Stock Compatibility**: Full stock `usertests` executed cleanly on the patched kernel (`ALL TESTS PASSED`).
- **Persistence Testing**: Multi-boot durability verified using random 96-bit cryptographic tokens across standard reboots, fresh-disk negative controls, and abrupt SIGKILL terminations without explicit user `sync`.
- **Adversarial Evaluation**: 9 attack scenarios (`atest.c`) blocked with explicit canonical log verification.
- **Stress Testing**: 320 independent multi-core concurrency stress trials executed with zero false negatives observed.

