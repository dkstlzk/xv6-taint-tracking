# Cross-Member Kernel Denial-Path Security Audit (logs/audit.md)

The historical headings below called the three members' contributions separate
passes. They collectively belong to audit pass #1. The actual Day 6 pass #2 is
recorded at the end; final pass #3 remains a Day 9 task. Assigned reviewers are
ownership labels, not fabricated human signatures.

Audit passes per CSL3030 Project 3 Plan (§13, §14) and Handbook Chapter 25.
Each denial check is audited against the seven obligations:
1. Locks held and properly released before returning.
2. Transactions begun and closed via `end_op()`.
3. Inode references dropped via `iput()` (or `iunlockput()` if locked).
4. Memory buffers released.
5. Canonical log string printed exactly once.
6. Error code `-1` returned to user space.
7. System invariants preserved.

---

## Pass 1: Member B Audits Member D Enforcement Sites (Task B-10)

| Site | File & Line | Lock State on Refusal | Log String Emitted | Return Value | Status | Auditor Notes |
|---|---|---|---|---|---|---|
| Pre-lock exec check | `kernel/exec.c:51` | `ip` unlocked; `iput(ip)`, `end_op()` called | `taint: exec refused for pid %d\n` | `-1` | PASS | Avoids lock inversion with `/safe`; releases transaction before exit |
| Post-lock exec check | `kernel/exec.c:62` | `ip` locked; `iunlockput(ip)`, `end_op()` called | `taint: exec refused for pid %d\n` | `-1` | PASS | Unlocks and decrements refcount properly; checks `T_FILE` & untrusted |
| Directory containment | `kernel/taint.c:13` | No locks on entry; `dp` locked & released via `iunlockput(dp)` | None (helper query) | `0` or `1` | PASS | Explicitly skips `.`, `..` and rejects self-match `dp->inum == ip->inum` |
| Pipe watermark tracking | `kernel/pipe.c:106` | `pi->lock` held during update and released | None | `i` or `-1` | PASS | Updates `tainted_hi` monotonically; reader check before copy preserves safety |

---

## Pass 2: Member C Audits Member B Enforcement Sites (Task C-12)

| Site | File & Line | Lock State on Refusal | Log String Emitted | Return Value | Status | Auditor Notes |
|---|---|---|---|---|---|---|
| Filewrite write-down gate | `kernel/file.c:166` | `f->ip` locked; `iunlock(f->ip)`, `end_op()` called | `taint: write refused for pid %d\n` | `-1` | PASS | Proper lock and transaction cleanup on write denial; allows console & pipe |
| Fileread taint latch | `kernel/file.c:124` | `f->ip` locked during check and update | None | bytes read | PASS | Only taints if `r > 0`; 0-byte read leaves process clean |

---

## Pass 3: Member D Audits Member C Enforcement Sites (Task D-10)

| Site | File & Line | Lock State on Refusal | Log String Emitted | Return Value | Status | Auditor Notes |
|---|---|---|---|---|---|---|
| `sys_open` write gate | `kernel/sysfile.c:371` | `ip` locked; `iunlockput(ip)`, `end_op()` called | `taint: write refused for pid %d\n` | `-1` | PASS | Precedes `itrunc(ip)` preventing dirty truncation of clean file |
| `sys_unlink` refusal | `kernel/sysfile.c:214` | Pre-transaction check; no locks held | `taint: unlink refused for pid %d\n` | `-1` | PASS | Refusal precedes `begin_op()`, zero log space wasted |
| `sys_link` refusal (OH1) | `kernel/sysfile.c:129` | Pre-transaction check; no locks held | `taint: link refused for pid %d\n` | `-1` | PASS | Refusal precedes `begin_op()`, prevents hard link laundering |
| `create()` born untrusted | `kernel/sysfile.c:298` | Inode locked during allocation and setup | None | `ip` | PASS | Sets `ip->untrusted = 1` in `ialloc` branch only; never re-labels existing file |
| `sys_marktaint` | `kernel/sysfile.c:543` | `ip` locked; `iupdate(ip)`, `iunlockput(ip)`, `end_op()` | None | `0` or `-1` | PASS | Validates `T_FILE`, path length, non-empty; durable update |

## Day 6 pass #2 — X-06, maintained by C

Reviewed implementation: `d64c43f` (kernel fix `fd3ac9d`); subsequent A-only demo
changes do not alter these sites. Automated execution plus assisted source
review; the team may add its own signatures after review.

| Assigned reviewer → owner | Site | Locks | Transaction | Inode refs | Buffers | Log / return | Preserved invariant and evidence |
|---|---|---|---|---|---|---|---|
| C → B | filewrite denial | ilock then iunlock | begin_op/end_op balanced | retained by open file, not leaked | none allocated | one write-refused / -1 | offset/data unchanged; T11a, A2 |
| C → B | fileread latch | inode lock covers metadata/read | read-only, none begun | retained by open file | caller-owned | no refusal; read count | only positive successful reads latch; T3 |
| D → C | sys_open denial | iunlockput | end_op before return | acquired ref dropped | file/fd not yet allocated | one write-refused / -1 | check precedes itrunc and leaves existing trust unchanged; T18/T19/A17/A18 |
| D → C | unlink/link gates | none acquired | before begin_op | none acquired | none | one unlink/link-refused / -1 | namespace unchanged; T9/A1/A3/A4/A8 |
| D → C | create non-regular gate | before nameiparent/ilock | caller mkdir/mknod closes its transaction | none acquired | none | one write-refused; create returns 0, syscall -1 | before ialloc; OH2 now passes after real GDB diagnosis |
| D → C | create regular / marktaint | ilock; iunlockput on exits | callers/end_op balanced | refs released | none | normal validation errors -1 without policy log | only new inode born untrusted; mark only T_FILE; T5/T16/T2 |
| B → D | exec pre-lock gate | target unlocked | end_op | iput target; in_safe releases directory | pagetable still 0 | one exec-refused / -1 | no target/directory lock inversion; T10/T20 |
| B → D | exec post-lock gate | iunlockput | end_op | target dropped | pagetable still 0 | one exec-refused / -1 | rejects valid untrusted ELF in safe; A16; preserves taint on allowed exec T12/A9 |
| B → D | in_safe | no target lock on entry; directory unlocked on all exits | uses caller transaction | iunlockput directory | stack dirent | query returns 0/1, no policy log | direct inode membership; excludes dot entries, root, directory itself |
| B → D | pipe read/write | one pipe spinlock; released for sleep and on return | none | pipe held by descriptors | one allocated pipe freed on final close | ordinary IO errors; no policy denial | update after copied byte; latch only after successful read; T6/T7/T8/T14, 60 seeded trials |

All seven denial obligations checked. The OH2 log uses the existing canonical
write-refused form, matching DEC-6. Ordinary invalid arguments, missing paths,
and failed IO are not policy refusals. CPUS=1/3 suite transcripts are in
`logs/G5/day6/`. No new lock/transaction/refcount defect was found in this pass.
