# Individual Submission Checklists (submissions/checklists.md)

**Authority**: CSL3030 Project 3 Plan (§19) and Final QA Specification (Appendix K)  
**Verification Tool**: `bash scripts/verify_individual.sh`  

---

## Member A Checklist (Python Agent, QEMU Bridge, Red-Team Attacks)
- [x] All assigned tasks (A-01 through A-24, X-05, X-10, X-11, X-12) completed.
- [x] Host unit tests pass (`python3 -m unittest discover -s agent_lab/tests`: 35 tests OK).
- [x] QEMU bridge features verified: random sentinel framing, timeout detection, crash recovery, persistent disk modes.
- [x] Red-team attack suite (`atest.c`) verified: 9 attacks blocked with canonical log lines.
- [x] Integration suite INT1–INT11 verified through real `QemuBridge`.
- [x] Individual package `submissions/member_A.zip` generated from immutable Git blobs and passes canonical byte verification.
- [x] Mock viva questions and drills prepared.

## Member B Checklist (System Calls, Process Taint, File Table Enforcement)
- [x] All assigned tasks (B-01 through B-14, X-02, X-04) completed.
- [x] System call ABI plumbing verified: `sys_hello`, `sys_marktaint`, `sys_istainted`.
- [x] Process taint lifecycle verified: `p->tainted = 0` on allocation/free, inherited across `kfork()`.
- [x] File table enforcement verified: positive-byte `fileread` latch, `filewrite` write-down prevention gate.
- [x] Stock `usertests` regressions passed cleanly on patched kernel at CPUS=1 and CPUS=3.
- [x] Individual package `submissions/member_B.zip` generated and verified.
- [x] Mock viva questions and drills prepared.

## Member C Checklist (Filesystem Persistence, Inode Allocation, Security Audits)
- [x] All assigned tasks (C-01 through C-19, X-03, X-06, X-07, X-09) completed.
- [x] Option C 64-byte `dinode` format verified with `NDIRECT 11` and static assertion.
- [x] Disk persistence verified: `iupdate` synchronous logging, T2 token persistence across reboot, abrupt kill-reboot persistence.
- [x] Inode allocation lifecycle verified: born-untrusted regular creation, clean reuse confirmed by inode equality (T13).
- [x] Denial-path audit passes #1, #2, #3 recorded in `logs/audit.md` with zero open defects or resource leaks.
- [x] False-positive table (FP-1 through FP-5) classified with zero bugs.
- [x] Individual package `submissions/member_C.zip` generated and verified.
- [x] Mock viva questions and drills prepared.

## Member D Checklist (Pipe Watermark, Path Containment, Concurrency Proofs)
- [x] All assigned tasks (D-01 through D-15, X-08, X-13) completed.
- [x] Pipe watermark progression (`tainted_hi`) verified under circular buffer wrap conditions.
- [x] Concurrency stress campaigns passed: 320 independent multi-core trials with zero false negatives observed.
- [x] Directory containment `/safe` verified: direct inode membership, rejects root, `.`, `..`, and self-match.
- [x] Exec containment verified: pre-lock `/safe` validation, post-lock untrusted ELF rejection (OH3).
- [x] Mathematical proofs and report draft sections completed.
- [x] Individual package `submissions/member_D.zip` generated and verified.
- [x] Mock viva questions and drills prepared.
