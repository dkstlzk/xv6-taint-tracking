# System Capabilities, Operational Limits & Architectural Trade-offs

Compiled for Task B-13, C-18, and D-14 (Milestone M8). Verified against measured test results and real host/guest execution.

---

## 1. Process Taint Irreversibility & Granularity (Member B)
- **Coarse Process-Granularity**: Process taint is irreversible and latching. Once a process reads even a single positive byte from an untrusted file (`f->ip->untrusted != 0`), `p->tainted = 1` remains set permanently for the remaining lifespan of that process (Invariant I1).
- **Inheritance Across Fork**: Child processes unconditionally inherit parent taint state during `kfork()`.
- **Permanent Shell Consequence (FP-5)**: If an interactive shell (`sh`) reads from an untrusted source or executes marked commands through a pipe (`cat /inbox.txt | sh`), the shell process itself becomes permanently tainted. Subsequent invocations of utilities outside `/safe` (`/ls`, `/rm`) are denied (`taint: exec refused`).

---

## 2. Pipe Watermark Tracking & Concurrency Bounds (Member D)
- **Monotonic Watermark Design**: Pipe taint propagation uses a single uint high-watermark `tainted_hi` within `struct pipe` (Invariant I5). When a tainted writer copies $k$ bytes into the buffer, `pi->tainted_hi = pi->nwrite` updates under `pi->lock`.
- **Conservative Taint Condition**: A reader is latched tainted if `(int)(pi->tainted_hi - pi->nread) > 0`. If clean bytes precede an unread tainted byte in the buffer, reading those clean bytes will conservatively taint the reader (intentional safety trade-off preventing under-tainting).
- **Signed-Distance Invariant**: The circular buffer wrap condition relies on signed modular arithmetic: as long as `(pi->nwrite - pi->nread) <= PIPESIZE (512)`, signed subtraction accurately reflects byte order. Stale mark wrap could only occur if $2^{31}$ clean bytes are passed without reading the unread marked byte.
- **Measured Concurrency Bounds**: Validated across 320 independent multi-core trials (`CPUS=3`) with two concurrent 1024-byte writers and a reader under idle and busy host CPU load. Zero false negatives observed.

---

## 3. Filesystem Persistence & Capacity Boundaries (Member C)
- **Option C 64-byte `dinode` Layout**: One `uint` direct block pointer was repurposed for the `uint untrusted` field (`NDIRECT 11`, down from stock 12).
- **Maximum File Size**: Exactly 11 direct blocks + 256 indirect blocks = 267 blocks ($267 \times 1024 = 273,408\text{ bytes}$). Compared to stock xv6 ($268\text{ blocks} = 274,432\text{ bytes}$), file capacity is reduced by exactly 1 block ($1024\text{ bytes}$).
- **On-Disk Compatibility**: `struct dinode` size remains exactly 64 bytes (`_Static_assert(sizeof(struct dinode) == 64)`), preserving inodes per block (`IPB = 16`). `mkfs/mkfs.c` zeroes all dinodes on initialization without code alterations.
- **Born-Untrusted Invariant**: New regular files created by a tainted process are born untrusted (`ip->untrusted = 1`). Existing files opened for write are never re-labeled; attempts to write clean files are blocked (Invariant I2).
- **Redirection Behavior (FP-3)**: Under stock shell redirection `cat /inbox.txt > /target.txt`, the clean child truncates `/target.txt` at `sys_open` before `cat` executes and reads the marked input. While the write-down is denied (`cat: write error`), pre-existing file content is truncated by the clean child. Appending (`>>`) preserves existing contents.
- **False-Positive Classification Table (FP-1 through FP-5)**:
  - FP-1: Reading untrusted files taints caller process (expected latching design).
  - FP-2: Writing untrusted bytes into pipe sets high watermark tainted_hi (expected safety design).
  - FP-3: Shell redirection truncates before cat reads marked input (stock shell open semantics; write-down denied).
  - FP-4: Clean child creates untrusted file when parent is tainted (born-untrusted invariant).
  - FP-5: Interactive shell tainted after cat /inbox.txt | sh (expected process taint permanence).
- **Crash Recovery & Abrupt Reboot**: Taint persistence survives abrupt QEMU termination (SIGKILL without explicit user `sync`) once `sys_marktaint` completes, because `iupdate(ip)` writes synchronously through the log.

---

## 4. Path-Level `/safe` Containment (Member D)
- **Direct Inode Containment**: Containment is based on directory inode entry membership, not naive string prefix matching.
- **Exclusions**: Special directory entries (`.`, `..`), the root directory (`/`), and `/safe` itself are strictly rejected from execution by tainted processes.
- **Administrative Responsibility**: Clean processes have permission to populate `/safe` (e.g. creating hard links or copying authorized binaries). The kernel protects against tainted processes escalating privilege, not against clean administrators misconfiguring `/safe`.

---

## 5. Agent Lab & QEMU Transport (Member A)
- **Framing & Buffer Limits**: The xv6 shell input buffer is 100 bytes. The QEMU bridge enforces `MAX_CMD_LEN = 80` to safely accommodate random sentinel framing and newlines without buffer overflow.
- **Two-Layer Security Architecture**:
  - **Layer 1 (Host Agent)**: Tool parameter validation and prompt injection interception (`validate_action` in Python).
  - **Layer 2 (Guest Kernel)**: Hardware-enforced process and inode taint tracking rejecting unauthorized write-downs and untrusted execution even if Layer 1 is bypassed.
- **Failure Recovery**: Transport desynchronization or command timeout automatically invokes `QemuBridge.restart()`, preventing corrupted console state across test cases.
