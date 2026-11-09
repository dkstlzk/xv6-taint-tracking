# Team Contribution & Subsystem Attribution Record (CONTRIBUTIONS.md)

**Project**: CSL3030 Operating Systems Lab — Project 3 (Q3: Taint-Following Capabilities on xv6-riscv)  
**Authority**: CSL3030 Execution Plan (§5, §19) and Handbook Edition 5 (§4, §5)  

---

## 1. Member Ownership & Responsibilities

| Role / Member | Subsystem Ownership | Primary Source & Test Files | Key Assigned Tasks | Key Deliverables & Artifacts |
| :--- | :--- | :--- | :--- | :--- |
| **Member A** | Python Agent, QEMU Bridge, Build Integration, Red-Team Attack Harness, Packaging | `agent_lab/`, `user/atest.c`, `Makefile`, `scripts/` | A-01–A-24, X-05, X-10, X-11, X-12 | QEMU bridge with sentinel recovery, 20 unit tests, 9 attacks in `atest.c`, INT1–INT11, packaging scripts, Gate G4/G6/G7 runner |
| **Member B** | System Calls, Process Lifecycle Taint, File Table Enforcement, Integration Coordination | `kernel/proc.h`, `kernel/proc.c`, `kernel/file.c`, `kernel/syscall.*`, `user/ttest.c` | B-01–B-14, X-02, X-04 | `p->tainted` lifecycle & `kfork()` inheritance, `fileread` latch, `filewrite` write-down gate, `ttest.c` harness, stock `usertests` regressions |
| **Member C** | Filesystem Persistence, 64-Byte Inode Layout, Inode Allocation, Denial-Path Security Audits | `kernel/fs.h`, `kernel/fs.c`, `kernel/sysfile.c`, `mkfs/mkfs.c`, `user/ftest.c` | C-01–C-19, X-03, X-06, X-07, X-09 | Option C 64-byte `dinode` persistence, `sys_marktaint`, `sys_open`/`unlink`/`link` write-down gates, OH2 non-regular create gate, `ftest.c` harness, `logs/audit.md` |
| **Member D** | Pipe Concurrency Watermark, `/safe` Path Containment, Exec Protection & Invariant Proofs | `kernel/pipe.h`, `kernel/pipe.c`, `kernel/exec.c`, `kernel/taint.c`, `user/ptest.c` | D-01–D-15, X-08, X-13 | Watermark monotonic progression (`tainted_hi`), `in_safe` directory containment, pre-lock & post-lock `exec` containment, `ptest.c`, 320 stress trials |

---

## 2. File-by-File Attribution Matrix

| File Path | Primary Owner | Secondary / Co-authors | Description of Changes |
| :--- | :--- | :--- | :--- |
| `kernel/proc.h` | Member B | Member D | Added `int tainted;` field to `struct proc`. |
| `kernel/proc.c` | Member B | Member A | Initialized `p->tainted = 0` in `allocproc`/`freeproc`; copied taint in `kfork` under lock. |
| `kernel/defs.h` | Member B / D / C | Member A | Prototypes for syscalls (`hello`, `marktaint`, `istainted`), taint query helpers (`in_safe`, `ip_untrusted`). |
| `kernel/syscall.h` / `syscall.c` | Member B | — | System call numbers (22, 23, 24) and dispatch table entries for `hello`, `marktaint`, `istainted`. |
| `kernel/sysproc.c` | Member B | — | Implemented `sys_hello()` and `sys_istainted()`. |
| `kernel/file.c` | Member B | Member C | Implemented positive-byte `fileread` taint latch and `filewrite` write-down prevention gate. |
| `kernel/fs.h` | Member C | — | Repurposed 1 direct pointer for `uint untrusted` (`NDIRECT 11`); static assertion for 64-byte `dinode`. |
| `kernel/fs.c` | Member C | — | Inode `untrusted` disk persistence across `ilock`, `iupdate`, `iput`, and `ialloc` reuse zeroing. |
| `kernel/sysfile.c` | Member C | Member D | Implemented `sys_marktaint()`, `sys_open` gate before truncation, `sys_unlink`, `sys_link` (OH1), and `create` (OH2). |
| `kernel/pipe.h` / `pipe.c` | Member D | — | Added `tainted_hi` watermark to `struct pipe`; monotonic per-byte update and reader latching. |
| `kernel/taint.h` / `taint.c` | Member D | Member C | Implemented `in_safe()` directory inode traversal and `ip_untrusted()` helper. |
| `kernel/exec.c` | Member D | Member B | Implemented pre-lock directory check and post-lock untrusted ELF rejection (OH3). |
| `user/ttest.c` | Member B | — | Functional syscall and process isolation tests T0, T1, T3, T4, T11a. |
| `user/ftest.c` | Member C | Member A | Filesystem security and persistence tests T2, T5, T9, T11b, T13, T16, T17, T18, T19, OH2, FP4. |
| `user/ptest.c` | Member D | — | Pipe watermark and execution containment tests T6, T7, T8, T10, T12, T14, T20, concurrent stress. |
| `user/atest.c` | Member A | — | Red-team attack suite A1, A2, A3, A4, A8, A9, A16, A17, A18. |
| `agent_lab/` | Member A | — | QEMU bridge, deterministic and gullible planners, tool validation, host test suite. |
| `scripts/` | Member A | Member B, C, D | Evidence header generator, regression runner, persistence orchestrator, packaging and verification. |
| `Makefile` | Member A | Member B, C, D | Build wiring for stubs, `taint.o`, `fs.img` dependencies, and `STOCK_UPROGS` vs `PROJECT_UPROGS`. |

---

## 3. Cross-Member Review & Audit Distribution
- **Ring Security Audits (`logs/audit.md`)**:
  - Member C audited Member B's enforcement sites (`kernel/file.c`).
  - Member D audited Member C's enforcement sites (`kernel/sysfile.c`).
  - Member B audited Member D's enforcement sites (`kernel/pipe.c`, `kernel/exec.c`, `kernel/taint.c`).
  - Complete ring passes verified at Gate G3 (Pass #1), Gate G5 (Pass #2), and Gate G6 (Pass #3).
- **Gate Coordination**:
  - Gate G1: Member B (tag `m1-plumbing`)
  - Gate G2: Member C
  - Gate G3: Member D (tag `m2-vertical`)
  - Gate G4: Member A (tag `m3-ai-ready`)
  - Gate G5: Member D (tag `g5-mandatory`)
  - Gate G6: Member B (tag `g6-regression`)
  - Gate G7a: Member A (tag `v1.0-freeze`)
  - Gate G7b: Member D (tag `v1.0-submission`)

