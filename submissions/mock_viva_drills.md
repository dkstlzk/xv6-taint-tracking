# Mock Viva Questions & Technical Drills (submissions/mock_viva_drills.md)

Prepared for Task A-24, B-14, C-19, D-15, and X-13 per CSL3030 Project 3 Execution Plan.

---

## Member A: Python Agent, QEMU Bridge, Red-Team Attacks
1. **Q**: How does the QEMU bridge prevent command desynchronization with the xv6 shell?
   - **A**: The bridge uses random 8-character hexadecimal sentinels (`@<token>@`). Each command is dispatched followed by `echo @<token>@`. The bridge accumulates guest serial output until it matches `\n@<token>@\n\$ `, ensuring the shell command completed and the prompt is ready for the next command. Timeouts trigger clean restart recovery.
2. **Q**: Why is `MAX_CMD_LEN` restricted to 80 characters?
   - **A**: In `user/sh.c`, the shell input buffer is sized at 100 bytes. With an 80-character payload, sentinel framing (`echo @12345678@`), and newline, the line fits comfortably under 100 bytes without truncation or stack corruption.
3. **Q**: How does Attack A8 bypass user-level wrappers?
   - **A**: A8 invokes raw assembly ecall (`li a7, 18`) directly attempting to unlink `/important.txt`. The kernel gate in `kernel/sysfile.c:sys_unlink` catches the attempt regardless of user library wrappers because it checks `myproc()->tainted`.

---

## Member B: System Calls, Process Taint, File Table Gates
1. **Q**: Exactly where and when is `p->tainted` set during `fileread()`?
   - **A**: In `kernel/file.c:fileread()`, after `readi(f->ip, ...)` returns the number of bytes read $r$, if $r > 0$ and `f->ip->untrusted` is non-zero, the kernel sets `myproc()->tainted = 1`. If $r = 0$ (e.g. EOF on an empty file), the process remains clean.
2. **Q**: How is process taint propagated to child processes?
   - **A**: In `kernel/proc.c:kfork()`, after allocating the new child process control block `np`, the parent's taint state is copied under `np->lock`: `np->tainted = p->tainted;` before `np->state` is set to `RUNNABLE`.
3. **Q**: Why does `sys_hello` take 4 arguments and return 10?
   - **A**: `sys_hello()` extracts four integer arguments (`argint(0, &a)`, etc.) and prints `hello %d %d %d %d\n`. It computes and returns the sum ($1 + 2 + 3 + 4 = 10$) as a smoke test of the xv6 RISC-V syscall ABI.

---

## Member C: Filesystem Persistence & 64-Byte Dinode Layout
1. **Q**: How was `untrusted` added to `struct dinode` without changing its size?
   - **A**: In stock xv6, `struct dinode` had 12 direct block addresses (`NDIRECT = 12`) plus 1 indirect block address. We reduced `NDIRECT` to 11 and added `uint untrusted`. Total size is:
     $$\text{type}(2) + \text{major}(2) + \text{minor}(2) + \text{nlink}(2) + \text{size}(4) + \text{addrs}(11 + 1)\times 4 + \text{untrusted}(4) = 64\text{ bytes}$$
     Because $64\text{ bytes}$ is unchanged, inodes per block ($\text{IPB} = \text{BSIZE}/64 = 16$) is preserved, and `mkfs.c` requires zero code modifications.
2. **Q**: What is the impact of `NDIRECT 11` on maximum file size?
   - **A**: Maximum file size is $(11 + 256) \times 1024 = 267\text{ blocks} = 273,408\text{ bytes}$ (1 block less than stock xv6).
3. **Q**: How does T13 confirm inode reuse?
   - **A**: T13 creates `/r.txt`, retrieves its inode number via `fstat()`, marks it untrusted, and unlinks it. It then creates 8 fresh files. When a newly created file matches the exact original inode number (`old.ino == new.ino`), it verifies that reading the new file leaves the process clean, proving taint was reset upon deallocation.

---

## Member D: Pipe Watermark & Exec Path Containment
1. **Q**: Explain the pipe watermark calculation `(int)(pi->tainted_hi - pi->nread) > 0`.
   - **A**: `pi->tainted_hi` records the cumulative byte offset of the latest tainted write. As readers consume bytes, `pi->nread` advances. As long as `pi->tainted_hi - pi->nread > 0`, unread tainted bytes remain in the pipe buffer or were consumed by this read, so the reader becomes tainted. Signed subtraction correctly handles 32-bit modular arithmetic.
2. **Q**: How does `in_safe()` determine directory containment?
   - **A**: `in_safe(struct inode *ip)` looks up `/safe` via `namei("/safe")`, acquires its lock, and scans its directory entries using `readi()`. If any entry's `inum` equals `ip->inum`, and the entry is not `.` or `..`, it returns 1. It explicitly rejects self-matches (`ip->inum == dp->inum`) so `/safe` itself cannot be executed.
3. **Q**: Why must `in_safe()` be called before locking the target binary inode in `exec.c`?
   - **A**: `in_safe()` needs to lock the `/safe` directory inode to read its entries. If `exec.c` locked the target binary inode first, concurrent lookups traversing from `/safe` to the binary could encounter lock inversion deadlocks. Checking `in_safe()` before locking the target eliminates this lock hierarchy issue.
