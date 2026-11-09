# Individual Submission Checklists (submissions/checklists.md)

## Member C Checklist (Filesystem Persistence, Inode Allocation, Security Audits)
- [x] All assigned tasks (C-01 through C-19, X-03, X-06, X-07, X-09) completed.
- [x] Option C 64-byte `dinode` format verified with `NDIRECT 11` and static assertion.
- [x] Disk persistence verified: `iupdate` synchronous logging, T2 token persistence across reboot, abrupt kill-reboot persistence.
- [x] Inode allocation lifecycle verified: born-untrusted regular creation, clean reuse confirmed by inode equality (T13).
- [x] Denial-path audit passes #1, #2, #3 recorded in `logs/audit.md` with zero open defects or resource leaks.
- [x] False-positive table (FP-1 through FP-5) classified with zero bugs.
- [x] Individual package `submissions/member_C.zip` generated and verified.
- [x] Mock viva questions and drills prepared.
