# Two-Layer Taint Tracking & Kernel Security Audit Trail on xv6-riscv

**Course**: CSL3030 Operating Systems (Project 3)  
**Target Platform**: 64-bit RISC-V (`qemu-system-riscv64`, `virt` machine, `smp=3`)  
**Base System**: MIT xv6-riscv (`f780d7850765d74cbdcd1fff4b822d8829f7c9bd`)  
**Submission Artifacts**: Complete Project Archive (`os-project-complete.zip`) & Verification Report  

---

## 1. Introduction & Problem Statement

Autonomous AI coding agents and LLM-driven terminal interfaces are increasingly granted shell and execution access to develop, test, and debug operating systems. However, autonomous agents are susceptible to **prompt injection attacks**: malicious, untrusted data inside a repository (e.g., adversarial test cases, untrusted configuration files, or data files) can trick the model into executing unauthorized system operations, such as deleting critical source files (`unlink`), corrupting system state (`truncate`, write-down), or executing arbitrary untrusted binaries (`exec`).

To address this challenge without relying purely on fallible model self-policing, this project implements a **Two-Layer Defense Architecture**:
1. **Layer 1 (Host-Side Agent Validation & Dispatcher)**: A Python host framework enforcing schema validation, argument allowlisting, and parameter sanitation before commands are transmitted to QEMU.
2. **Layer 2 (Kernel-Level Information-Flow Taint Tracking & Security Audit Trail)**: Hardware-enforced kernel isolation in xv6-riscv that tracks the flow of untrusted data across memory, disk inodes, and IPC channels, strictly preventing write-downs and unauthorized binary execution, and recording tamper-evident audit events in a kernel ring buffer.

---

## 2. Implementation Details

```
                         [ USER / EXTERNAL INPUT ]
                                     │
                                     ▼
                     ┌───────────────────────────────┐
                     │    Layer 1: Host Agent Lab    │
                     │  - Action & Schema Validation │
                     │  - Parameter Allowlisting     │
                     │  - Read-Only Command Gating   │
                     └───────────────┬───────────────┘
                                     │ (Commands via QEMU Bridge)
                                     ▼
                     ┌───────────────────────────────┐
                     │   Layer 2: xv6-riscv Kernel   │
                     │  - Invariant I1: Proc Taint   │
                     │  - Invariant I2: Write Gate   │
                     │  - Invariant I3: Inode Persist│
                     │  - Invariant I4: Safe Exec    │
                     │  - Invariant I5: Pipe Watermrk│
                     └───────────────┬───────────────┘
                                     │ (Security Violations & State Changes)
                                     ▼
                     ┌───────────────────────────────┐
                     │  Kernel Security Audit Trail  │
                     │  - Bounded 64-event Ring Buffer│
                     │  - Monotonic 1-based Sequences│
                     │  - Lock-safe copyout() to user│
                     │  - sys_gettaintlog() / taintlog│
                     └───────────────────────────────┘
```

### 2.1 Process Taint Tracking (Invariant I1)
Every process structure (`struct proc`) includes a flag `p->tainted`. A clean process becomes tainted when it reads $k > 0$ bytes from an untrusted inode (`ip->untrusted != 0`). Process taint is **irreversible**: once tainted, it cannot be cleared for the duration of the process. In `kfork()`, child processes inherit the parent's taint state under `np->lock`.

### 2.2 Strict Write-Down Prevention (Invariant I2)
A tainted process is forbidden from modifying clean files on disk. The kernel enforces this at multiple system call gates:
- **`filewrite()`**: Rejects writes from tainted processes to clean regular files (`ip->type == T_FILE && !ip->untrusted`), returning `-1`.
- **`sys_open()`**: Rejects opening clean files with `O_WRONLY`, `O_RDWR`, or `O_TRUNC` before `itrunc()` can truncate blocks.
- **`sys_unlink()` & `sys_link()`**: Prohibits tainted processes from unlinking clean files or creating hard-link aliases to bypass security labels.
- **`create()`**: Prohibits tainted processes from creating non-file directory structures.

### 2.3 Filesystem Inode Persistence (Invariant I3)
File taint state persists durably across system reboots. To maintain on-disk structure compatibility without breaking disk layouts, we utilized the 64-byte Option C inode structure:
- Repurposed one unused address slot in `struct dinode` to store `uint untrusted`, keeping `sizeof(struct dinode) == 64` and `NDIRECT == 11`.
- Any regular file created by a tainted process is marked **born-untrusted** (`ip->untrusted = 1`).
- Changes are committed durably using xv6's journal log via `iupdate(ip)`.
- When an inode is unlinked and freed, its attributes are zeroed in `ialloc()`, preventing phantom taint on reallocation.

### 2.4 Path-Based Execution Containment (Invariant I4)
Tainted processes are forbidden from executing arbitrary executables. Execution is permitted only if:
1. The executable is located directly inside the `/safe` directory (`in_safe(ip)` checks parent directory inode membership, rejecting `.`, `..`, and `/safe` self-matches).
2. The binary itself is verified clean (`!ip_untrusted(ip)`), preventing attackers from laundering untrusted binaries into `/safe`.

### 2.5 Pipe Watermark Tracking (Invariant I5)
Inter-process communication via pipes tracks untrusted data flow concurrently:
- Each `struct pipe` maintains a monotonic 32-bit byte offset `tainted_hi` representing the highest watermark of untrusted data written.
- When reading from the pipe, the reader checks the signed difference $\Delta = (tainted\_hi - nread) \pmod{2^{32}}$. If $\Delta > 0$, the bytes being consumed were tainted, and the reader process transitions to `p->tainted = 1`.

### 2.6 Kernel Security Audit Trail (Extension)
To enable tamper-evident monitoring, a kernel security audit buffer was implemented:
- **Ring Buffer (`audit.h`, `taint.c`)**: A fixed 64-slot ring buffer protected by `audit_lock`.
- **Logged Events**: `AUDIT_PROCESS_TAINTED` (1), `AUDIT_FORK_PROPAGATION` (2), `AUDIT_PIPE_PROPAGATION` (3), `AUDIT_UNLINK_DENIED` (4), `AUDIT_EXEC_DENIED` (5), and `AUDIT_WRITE_DOWN_DENIED` (6).
- **Concurrency & Lock Ordering**: Events are snapshotted to a local buffer while holding `audit_lock`. The lock is released **before** calling `copyout()` to user space, avoiding deadlock with page tables.
- **System Call & Tooling**: System call `gettaintlog(buf, max_events)` and user command `taintlog` inspect the audit trail chronologically.

---

## 3. Challenges Faced & Engineering Solutions

1. **Lock Ordering and Deadlock Prevention**:
   - *Problem*: In `exec.c`, resolving directory containment for `/safe` via `namei()` while holding inode sleeplocks caused kernel lock inversions and panics.
   - *Solution*: Segregated `/safe` validation into two phases: pre-lock path checking (`in_safe()` without holding locks on `ip`), followed by post-lock verification (`ip->type == T_FILE && !ip_untrusted(ip)`).

2. **Pipe Concurrency & Integer Wrap-Around**:
   - *Problem*: In high-throughput streaming workloads where multiple clean and tainted processes write to a 512-byte pipe, integer overflow could lead to false-negative taint leaks.
   - *Solution*: Implemented high-watermark tracking with signed 32-bit modular arithmetic. Verified through 320 independent multicore stress trials (`smp=3`) with zero false negatives.

3. **Inode Reuse & Phantom Taints**:
   - *Problem*: If an untrusted file is deleted and its disk inode is reallocated to a clean process, residual taint flags in `dinode` could incorrectly taint clean files.
   - *Solution*: Added explicit clearing in `ialloc()` and verified through inode equality tests (T13), confirming that eight consecutive reuses of inode 42 remained clean.

4. **Five-Argument `copyout()` in Audit Trail**:
   - *Problem*: During audit trail integration, `audit_getlog()` failed compilation due to incomplete `struct proc` definition when invoking xv6's five-argument `copyout(p->pagetable, p->sz, uaddr, src, len)`.
   - *Solution*: Corrected include ordering in `kernel/taint.c` by including `proc.h` before `defs.h`, preserving the custom five-argument copyout contract without modifying kernel memory layout.

---

## 4. Innovative Approaches Demonstrated

1. **Two-Layer Defense in Depth**: Rather than choosing between application-level safety and OS sandboxing, the project demonstrates a hybrid model where user-space agent validators filter obvious injection attacks, while kernel-space invariants enforce non-bypassable hardware security.
2. **Zero-Overhead Option C Dinode**: Repurposed an unused block address to store persistence metadata without altering disk block alignment or inode sizes, preserving backward filesystem compatibility.
3. **High-Watermark Pipe Tracking**: Avoided expensive linked-list byte tracking in kernel space by utilizing a single monotonic offset watermark that guarantees safe information-flow tracking under concurrent multicore execution.
4. **Lockless Copyout Snapshotting**: The audit trail decouples kernel event acquisition from user-space memory paging by taking a stack snapshot under `audit_lock` and releasing the spinlock before calling `copyout()`.

---

## 5. Verification & Test Results

All verification suites were executed directly on `qemu-system-riscv64` with clean passes across all test suites:

| Test Suite | Purpose | Test Cases | Result |
| :--- | :--- | :--- | :--- |
| **`ttest`** | System call plumbing & process taint latching | T0, T1, T3, T4, T11a | **PASS (5/5)** |
| **`ftest`** | Inode persistence, write-down denial, inode reuse | T5, T9, T11b, T13, T16, T18, T19, T-OH2 | **PASS (10/10)** |
| **`ptest`** | Pipe watermark tracking, multi-core stress, `/safe` exec | PIPE-0, PIPE-E, T6, T7, T8, T10, T12, T14, T20, STRESS | **PASS (15/15)** |
| **`atest`** | Red-team attack suite (adversarial injection, name laundering) | A1, A2, A3, A4, A8, A9, A16, A17, A18 | **PASS (9/9)** |
| **`audtest`** | Kernel audit trail verification, overflow, sequencing | AUD1–AUD9 (Baseline, Safety, Pipe, Denials, Monotonicity) | **PASS (13/13)** |
| **`taintlog`** | User-space inspection utility for audit ring buffer | Real-time sequence rendering and formatting | **PASS** |
| **`usertests`**| Full stock xv6 regression test suite | Comprehensive OS functionality regression | **ALL PASSED** |

### Known Limitations
- Process taint is monotonic at process granularity; fine-grained byte-level taint tracking inside process address space is not supported.
- Pipes use conservative watermarking: a clean write preceding an unread tainted byte will cause clean bytes to be conservatively classified as tainted.

---

## 6. Conclusion
The implementation successfully provides robust, hardware-enforced containment for autonomous agents running on xv6-riscv. Even in the presence of prompt injection and complete bypass of user-space LLM safeguards, the kernel invariants prevent unauthorized write-downs, restrict binary execution, persist security labels across boots, and maintain a tamper-evident audit log.
