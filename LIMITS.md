# System Capabilities, Operational Limits & Architectural Trade-offs

Compiled for Task B-13, C-18, and D-14 (Milestone M8). Verified against measured test results and real host/guest execution.

---

## 1. Process Taint Irreversibility, Partial-Read Tracking & Granularity (Member B)
- **Coarse Process-Granularity**: Process taint is irreversible and latching. Once any positive number of bytes from an untrusted file (`f->ip->untrusted != 0`) are copied into user space, `p->tainted = 1` remains set permanently for the remaining lifespan of that process (Invariant I1).
- **Discovered Partial-Read Failure & Fix**: Previously, `fileread()` checked only whether `readi()` returned `r > 0`. If `readi()` copied one or more filesystem blocks or partial page bytes via `copyout()` and then hit an unmapped/read-only boundary, `copyout()` returned `-1` and `readi()` returned `tot = -1`, discarding the byte count already delivered to user memory and failing to taint the process. The fixed read path (`copyout_partial()` in `kernel/vm.c`, `either_copyout_partial()` in `kernel/proc.c`, `readi_partial()` in `kernel/fs.c`, and `fileread()` in `kernel/file.c`) tracks actual bytes copied (`copied`). Any `copied > 0` from an untrusted inode taints the process—even when `read()` returns `-1`—while zero-byte reads and failed transfers with `copied == 0` leave clean processes clean.
- **Inheritance Across Fork**: Child processes unconditionally inherit parent taint state during `kfork()`.
- **Permanent Shell Consequence (FP-5)**: If an interactive shell (`sh`) reads from an untrusted source or executes marked commands through a pipe (`cat /inbox.txt | sh`), the shell process itself becomes permanently tainted. Subsequent invocations of utilities outside `/safe` (`/ls`, `/rm`) are denied (`taint: exec refused`).

---

## 2. Pipe Watermark Tracking & Concurrency Bounds (Member D)
- **Monotonic Watermark Design**: Pipe taint propagation uses a single uint high-watermark `tainted_hi` within `struct pipe` (Invariant I5). When a tainted writer copies $k$ bytes into the buffer, `pi->tainted_hi = pi->nwrite` updates under `pi->lock`.
- **Conservative Taint Condition**: A reader is latched tainted if `(int)(pi->tainted_hi - pi->nread) > 0` and at least one byte is copied out (`i > 0`). If clean bytes precede an unread tainted byte in the buffer, reading those clean bytes conservatively taints the reader (intentional safety trade-off preventing under-tainting under concurrent writers without per-byte tags). Zero-byte reads and failed `copyout` calls that copy zero bytes leave clean readers clean.
- **Signed-Distance Invariant**: The circular buffer wrap condition relies on signed modular arithmetic: as long as `(pi->nwrite - pi->nread) <= PIPESIZE (512)`, signed subtraction accurately reflects byte order. Stale mark wrap could only occur if $2^{31}$ clean bytes are passed without reading the unread marked byte.
- **Measured Concurrency Bounds**: Validated across 320 independent multi-core trials (`CPUS=3`) with two concurrent 1024-byte writers and a reader under idle and busy host CPU load. Zero false negatives observed.

---

## 3. Filesystem Persistence & Capacity Boundaries (Member C)
- **Option C 64-byte `dinode` Layout**: One `uint` direct block pointer was repurposed for the `uint untrusted` field (`NDIRECT 11`, down from stock 12).
- **Maximum File Size**: Exactly 11 direct blocks + 256 indirect blocks = 267 blocks ($267 \times 1024 = 273,408\text{ bytes}$). Compared to stock xv6 ($268\text{ blocks} = 274,432\text{ bytes}$), file capacity is reduced by exactly 1 block ($1024\text{ bytes}$).
- **On-Disk Compatibility & Mandatory Image Rebuild**: `struct dinode` size remains exactly 64 bytes (`_Static_assert(sizeof(struct dinode) == 64)`), preserving inodes per block (`IPB = 16`). However, because `NDIRECT` decreased from `12` to `11` and word offset 12 holds `uint untrusted`, **pre-existing or stock xv6 `fs.img` disk images are incompatible** and must be rebuilt cleanly with `make clean && make fs.img` (using `mkfs/mkfs`, which zeroes all dinodes). Loading an old stock `fs.img` would misinterpret an indirect/direct block address as a non-zero `untrusted` flag.
- **Born-Untrusted Invariant**: New regular files created by a tainted process are born untrusted (`ip->untrusted = 1`). Existing files opened for write are never re-labeled; attempts to write clean files are blocked (Invariant I2).
- **Redirection Behavior (FP-3)**: Under stock shell redirection `cat /inbox.txt > /target.txt`, the clean child truncates `/target.txt` at `sys_open` before `cat` executes and reads the marked input. While the write-down is denied (`cat: write error`), pre-existing file content is truncated by the clean child. Appending (`>>`) preserves existing contents.
- **Crash Recovery & Abrupt Reboot**: Taint persistence survives abrupt QEMU termination (SIGKILL without explicit user `sync`) once `sys_marktaint` completes, because `iupdate(ip)` writes synchronously through the log.

---

## 4. Path-Level `/safe` Containment (Member D)
- **Direct Inode Containment**: Containment resolves `TAINT_SAFE_DIR` (`"/safe"` in `kernel/taint.h`) dynamically via `namei()` and matches `(dp->dev == ip->dev && de.inum == ip->inum)` across direct directory entries—rather than naive string prefix matching or a fixed inode number. Hard links placed in `/safe` by a clean process are permitted only if the underlying target inode is a clean `T_FILE`.
- **Exclusions**: Special directory entries (`.`, `..`), subdirectories below `/safe`, the root directory (`/`), and `/safe` itself are strictly rejected from execution by tainted processes, as are untrusted binaries copied into `/safe` (post-lock `!ip_untrusted(ip)` check).
- **Administrative Responsibility**: Clean processes have permission to populate `/safe` (e.g. creating hard links or copying authorized binaries). The kernel protects against tainted processes escalating privilege, not against clean administrators misconfiguring `/safe`.

---

## 5. Agent Lab & QEMU Transport (Member A)
- **Framing & Buffer Limits**: The xv6 shell input buffer is 100 bytes (`user/sh.c`). The QEMU bridge enforces `MAX_CMD_LEN = 80` (`agent_lab/xv6agent/config.py`) to safely accommodate random sentinel framing and newlines without buffer overflow.
- **Two-Layer Security Architecture**:
  - **Layer 1 (Host Agent)**: Tool parameter validation and prompt injection interception (`validate_action` in Python, referencing `config.SAFE_DIR`).
  - **Layer 2 (Guest Kernel)**: Kernel-enforced process and inode taint tracking rejecting unauthorized write-downs and untrusted execution even if Layer 1 is bypassed.
- **Failure Recovery**: Transport desynchronization or command timeout automatically invokes `QemuBridge.restart()`, preventing corrupted console state across test cases.

---

## 6. Coupled Configuration Settings (Must Change Together)

When tuning parameters for evaluation or scaling, the following settings are strictly coupled across subsystems:

| Subsystem | Coupled Settings & Locations | Coupling Constraint / Consequence of Mismatch |
| :--- | :--- | :--- |
| **RAM** | `PHYSTOP` (`kernel/memlayout.h`: `KERNBASE + 128 * 1024 * 1024`) <-> QEMU `-m 128M` (`Makefile` `QEMUOPTS` & `agent_lab/xv6agent/bridge.py`) | Raising QEMU `-m` without raising `PHYSTOP` leaves added RAM unused; lowering QEMU `-m` below `PHYSTOP` causes physical memory faults during `kinit()` / `kalloc()`. `KSTACK` (`1 * PGSIZE`) in `memlayout.h` also bounds kernel stack frames such as `tmp[AUDIT_BUF_SIZE]` in `audit_getlog()` (`kernel/taint.c`). |
| **Disk Layout & Log** | `NDIRECT` (`11`), `MAXFILE` (`267`), `BSIZE` (`1024`), `struct dinode` (`kernel/fs.h`) <-> `FSSIZE` (`2000`), `MAXOPBLOCKS` (`10`), `LOGBLOCKS` (`30`), `NBUF` (`30`) (`kernel/param.h`) <-> `mkfs/mkfs.c` <-> `fs.img` | Changing `struct dinode` or `BSIZE` invalidates existing `fs.img` images (`make clean && make fs.img` required) and alters `IPB`/`MAXFILE`. `FSSIZE` must exceed all compiled `UPROGS` + metadata (`nmeta = 47`, currently `1382/2000` blocks used). Raising `MAXOPBLOCKS` requires updating `LOGBLOCKS` and `NBUF`, while `filewrite()` (`kernel/file.c`) bounds chunk size via `((MAXOPBLOCKS - 4) / 2) * BSIZE`. |
| **Test & Pipe Buffers** | `USERSTACK` (`1` page = `4096` B in `kernel/param.h`) <-> local arrays in `user/ptest.c`, `user/audtest.c`, `user/ttest.c` <-> `PIPESIZE` (`512` B in `kernel/pipe.c`) <-> `sh.c` (`100` B) & `MAX_CMD_LEN` (`80` in `config.py`) | Increasing `STRESS_TOTAL_BYTES` (`ptest.c`) or `AUDIT_BUF_SIZE` (`kernel/audit.h`) risks overflowing `USERSTACK` (`4096` B) or `KSTACK` unless heap (`sbrk`) or `USERSTACK` is increased together. `STRESS_WRITER_BYTES` (`1024 > PIPESIZE`) requires concurrent reader/writer draining. `MAX_CMD_LEN` plus sentinel must remain under `sh.c`'s `100`-byte `getcmd` buffer. |
| **Policy & Stress Runner** | `TAINT_SAFE_DIR` & `TAINT_REFUSE_*` (`kernel/taint.h`) <-> `SAFE_DIR` (`agent_lab/xv6agent/config.py`) <-> `scripts/stress_campaign.py --cpus --timeout` | Changing `/safe` or refusal strings must match across `kernel/taint.h`, `agent_lab/xv6agent/config.py`, and evidence regexes (`scripts/check_g5.py`, `scripts/integration.py`). High trial counts or CPU load in `stress_campaign.py` must scale `--timeout` alongside `--cpus`. |
