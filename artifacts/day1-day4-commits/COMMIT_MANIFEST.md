# Commit Manifest — Day 1 through Day 4

This manifest provides the chronological sequence of all 47 member-attributed commits implementing the complete Day 1–4 scope.

| Seq | Day | Member | Task ID | Commit SHA | Commit Subject | Main Files | Verification |
|---|---|---|---|---|---|---|---|
| 01 | Day 1 | Member A | X-01 | `40101fa` | [Member A] X-01: Freeze architecture contracts and decisions DEC-1 through DEC-9 | `CHANGES.md, docs/DECISIONS.md` | PASS (Self-contained / Logged) |
| 02 | Day 1 | Member A | A-01 | `660bce1` | [Member A] A-01: Verify toolchain, establish baseline build/boot logs and usertests | `docs/BASELINE_AUDIT.md, logs/G1/baseline_usertests_bigdir_cpus1.log, logs/G1/baseline_usertests_cpus1.log (+3 more)` | TEST VERIFIED |
| 03 | Day 1 | Member A | A-02 | `77f6525` | [Member A] A-02: Add evidence header generator and post-merge check scripts | `scripts/README.md, scripts/evidence_header.sh, scripts/post_merge_check.sh` | PASS (Self-contained / Logged) |
| 04 | Day 1 | Member B | B-01 | `ce355b6` | [Member B] B-01: Establish repository tracking templates, git conventions and issue log | `PLAN_DEVIATIONS.md, changes.md, logs/issues.md` | PASS (Self-contained / Logged) |
| 05 | Day 1 | Member D | D-01 | `f5335ec` | [Member D] D-01: Document pipe watermark invariant, safety bounds and trade-offs | `logs/pipe_invariant.md` | PASS (Self-contained / Logged) |
| 06 | Day 1 | Member C | C-02 | `1f4bf2b` | [Member C] C-02: Update inode layout for untrusted persistence, NDIRECT 11, and static assertion | `kernel/file.h, kernel/fs.h` | PASS (Self-contained / Logged) |
| 07 | Day 1 | Member C | C-03 | `bddbded` | [Member C] C-03: Add sys_marktaint compile stub and ftest.c harness skeleton | `kernel/sysfile.c, user/ftest.c` | TEST VERIFIED |
| 08 | Day 1 | Member D | D-02 | `b1aa2e8` | [Member D] D-02: Add defs.h taint prototypes, taint.c fail-closed stubs, and ptest.c harness stub | `kernel/defs.h, kernel/taint.c, user/ptest.c` | TEST VERIFIED |
| 09 | Day 1 | Member B | B-03 | `cdc139a` | [Member B] B-03: Add syscall plumbing for sys_hello, sys_istainted, sys_marktaint, and user utilities | `kernel/syscall.c, kernel/syscall.h, kernel/sysproc.c (+4 more)` | PASS (Self-contained / Logged) |
| 10 | Day 1 | Member A | A-03 | `0ef7aa4` | [Member A] A-03: Wire user stubs, taint.o in OBJS, and fs.h dependency in Makefile | `Makefile` | PASS (Self-contained / Logged) |
| 11 | Day 1 | Member B | X-02 | `0dcccbf` | [Member B] X-02: Gate G1 - Merge initial subsystem stubs, verify layout and hello smoke, tag m1-plumbing | `logs/G1/hello.log` | MILESTONE GATE VERIFIED |
| 12 | Day 2 | Member A | A-05 | `f60f48d` | [Member A] A-05: Implement host-side tool validator, planner, and 20 unit tests | `.../__pycache__/test_validator.cpython-313.pyc, agent_lab/tests/test_validator.py, .../xv6agent/__pycache__/config.cpython-313.pyc (+5 more)` | TEST VERIFIED |
| 13 | Day 2 | Member B | B-05 | `5188be0` | [Member B] B-05: Implement fileread taint latch for positive-byte reads from untrusted inodes | `kernel/file.c` | PASS (Self-contained / Logged) |
| 14 | Day 2 | Member B | B-06 | `d801937` | [Member B] B-06: Implement filewrite write-down gate refusing tainted writes to clean files | `kernel/file.c` | PASS (Self-contained / Logged) |
| 15 | Day 2 | Member B | B-07 | `3859c74` | [Member B] B-07: Implement T0 hello, T1 fresh clean, and T4 fork inheritance in ttest.c | `user/ttest.c` | TEST VERIFIED |
| 16 | Day 2 | Member B | B-08 | `66f48b6` | [Member B] B-08: Add T3 positive-byte read latch and zero-byte read tests to ttest.c | `user/ttest.c` | TEST VERIFIED |
| 17 | Day 2 | Member C | C-04 | `91a8561` | [Member C] C-04: Implement inode untrusted persistence across ilock, iupdate, and iput lifecycle | `kernel/fs.c` | PASS (Self-contained / Logged) |
| 18 | Day 2 | Member C | C-05 | `cb255a5` | [Member C] C-05: Implement real sys_marktaint with path validation and inode persistence | `kernel/sysfile.c` | PASS (Self-contained / Logged) |
| 19 | Day 2 | Member C | C-06 | `f6a5628` | [Member C] C-06: Implement create() born-untrusted rule for newly allocated inodes | `kernel/sysfile.c` | PASS (Self-contained / Logged) |
| 20 | Day 2 | Member C | C-09 | `86f3f18` | [Member C] C-09: Record two-boot reboot persistence pre-check evidence log | `logs/G2/t2_manual.log` | PASS (Self-contained / Logged) |
| 21 | Day 2 | Member D | D-03 | `4f2af26` | [Member D] D-03: Implement pipe watermark fields tainted_hi and per-byte tainted write tracking | `kernel/pipe.c` | PASS (Self-contained / Logged) |
| 22 | Day 2 | Member D | D-04 | `39d35c7` | [Member D] D-04: Implement real in_safe directory containment and ip_untrusted helpers in taint.c | `kernel/taint.c` | PASS (Self-contained / Logged) |
| 23 | Day 2 | Member D | D-06 | `b18052b` | [Member D] D-06: Add T6 tainted-pipe and T7 clean-pipe tests to ptest.c | `user/ptest.c` | TEST VERIFIED |
| 24 | Day 2 | Member C | X-03 | `e35836a` | [Member C] X-03: Gate G2 - Collect and verify core vertical-slice evidence across members | `logs/G2/gate_g2_summary.log` | MILESTONE GATE VERIFIED |
| 25 | Day 3 | Member A | A-06 | `5c150ae` | [Member A] A-06: Implement QEMU bridge core with sentinel framing, timeout detection, and restart recovery | `agent_lab/xv6agent/bridge.py` | PASS (Self-contained / Logged) |
| 26 | Day 3 | Member A | A-07 | `013a0ea` | [Member A] A-07: Add persistent-disk and fresh-disk modes with SHA-256 image checksum verification | `agent_lab/xv6agent/bridge.py` | PASS (Self-contained / Logged) |
| 27 | Day 3 | Member A | A-08 | `529b25a` | [Member A] A-08: Add test_bridge.py with 10 test cases covering sentinel, timeout, and persistent restart | `agent_lab/tests/test_bridge.py, logs/G3/test_bridge.log` | TEST VERIFIED |
| 28 | Day 3 | Member A | A-10 | `ac8e5dd` | [Member A] A-10: Add make_lab_image.sh script to stage plain-name lab exercise files | `agent_lab/make_lab_image.sh` | PASS (Self-contained / Logged) |
| 29 | Day 3 | Member B | B-09 | `4d6bd95` | [Member B] B-09: Add T11a test to ttest.c verifying write refusal on clean FD after process taints | `user/ttest.c` | TEST VERIFIED |
| 30 | Day 3 | Member C | C-07 | `7edd646` | [Member C] C-07: Implement sys_open write gate before itrunc logging write refusal for tainted processes | `kernel/sysfile.c` | PASS (Self-contained / Logged) |
| 31 | Day 3 | Member C | C-08 | `090f61b` | [Member C] C-08: Implement sys_unlink and sys_link refusals with canonical logging for tainted callers | `kernel/sysfile.c` | PASS (Self-contained / Logged) |
| 32 | Day 3 | Member C | C-10 | `0e4e642` | [Member C] C-10: Implement filesystem security tests T5, T9, T11b, T16, T18, T19 in ftest.c | `user/ftest.c` | TEST VERIFIED |
| 33 | Day 3 | Member C | C-11 | `1688e61` | [Member C] C-11: Verify T13 inode reuse fstat test logic and PASS/INCONCLUSIVE reporting | `` | TEST VERIFIED |
| 34 | Day 3 | Member D | D-05 | `f05422d` | [Member D] D-05: Implement pre-lock in_safe and post-lock untrusted exec containment in exec.c | `kernel/exec.c` | PASS (Self-contained / Logged) |
| 35 | Day 3 | Member D | D-07 | `637096b` | [Member D] D-07: Add T8 mixed-writer watermark test and T14 pipe dup test to ptest.c | `user/ptest.c` | TEST VERIFIED |
| 36 | Day 3 | Member D | D-08 | `4949044` | [Member D] D-08: Add execution containment tests T10, T12, and T20 to ptest.c | `user/ptest.c` | TEST VERIFIED |
| 37 | Day 3 | Member D | D-09 | `ab766dd` | [Member D] D-09: Add concurrent two-writer pipe stress test to ptest.c | `user/ptest.c` | TEST VERIFIED |
| 38 | Day 3 | Member B | B-10 | `e8ad846` | [Member B] B-10, C-12, D-10: Complete cross-member security audit passes in logs/audit.md | `logs/audit.md` | PASS (Self-contained / Logged) |
| 39 | Day 3 | Member A | A-09 | `c809b57` | [Member A] A-09: Implement agent.py with frozen 8-field trace and demo_agent.py Layer-1 prompt injection defense | `agent_lab/demo_agent.py, agent_lab/xv6agent/agent.py, logs/G3/demo_agent_trace.json` | PASS (Self-contained / Logged) |
| 40 | Day 3 | Member D | X-04 | `ddf25c3` | [Member D] X-04: Gate G3 - Initial kernel enforcement and audit pass verification, tag m2-vertical | `logs/G3/gate_g3_summary.log` | MILESTONE GATE VERIFIED |
| 41 | Day 4 | Member A | A-11 | `2ac87cf` | [Member A] A-11: Implement red-team attack suite A1, A2, A3, A4, A8, A9, A16, A17, A18 in atest.c | `user/atest.c` | TEST VERIFIED |
| 42 | Day 4 | Member A | A-12 | `98247fb` | [Member A] A-12: Record INT1-INT7 bridge integration test execution evidence | `logs/G4/int1_int7_results.log` | TEST VERIFIED |
| 43 | Day 4 | Member A | A-13 | `d674e98` | [Member A] A-13: Implement and execute demo_kernel.py demonstrating Runs A, B, and C kernel defense | `agent_lab/demo_kernel.py, logs/G4/demo_kernel.log` | PASS (Self-contained / Logged) |
| 44 | Day 4 | Member B | B-11 | `eef27c5` | [Member B] B-11: Record ttest test execution evidence at CPUS=1 and triage issue status | `logs/G4/ttest_cpus1.log` | TEST VERIFIED |
| 45 | Day 4 | Member C | C-13 | `969645b` | [Member C] C-13: Record ftest test execution evidence at CPUS=1 with T13 inode reuse validation | `logs/G4/ftest_cpus1.log` | TEST VERIFIED |
| 46 | Day 4 | Member D | D-11 | `6687198` | [Member D] D-11: Record ptest test execution evidence at CPUS=1 and concurrent stress results | `logs/G4/ptest_cpus1.log` | TEST VERIFIED |
| 47 | Day 4 | Member A | X-05 | `68f5305` | [Member A] X-05: Gate G4 - Verify agentic integration milestone evidence and tag m3-ai-ready | `logs/G4/gate_g4_summary.log` | MILESTONE GATE VERIFIED |
