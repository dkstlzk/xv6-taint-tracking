# Day 1–Day 4 Final Engineering & Verification Report

**Project**: xv6 Taint-Tracking System & Agentic Lab (CSL3030 Project 3)  
**Milestones Completed**: Day 1, Day 2, Day 3, Day 4 (Gates G1, G2, G3, G4)  
**Milestone Tags**: `m1-plumbing`, `m2-vertical`, `m3-ai-ready`  
**Repository Working Copy**: `os-project`  
**Target Architecture**: RISC-V (`xv6-riscv`)  

---

## 1. Executive Summary

All tasks scheduled for Days 1 through 4 across all four team members (A, B, C, D) have been completely implemented, integrated, statically audited, unit-tested, and structured into a 47-commit member-wise Git history.

- **Member A**: Toolchain auditing, baseline evidence, Makefile wiring, QEMU bridge core, persistent disk checksums, 20 validator unit tests, 10 bridge tests, staging scripts (`make_lab_image.sh`), 8-field trace agent (`agent.py`), Layer-1 prompt injection defense (`demo_agent.py`), red-team adversarial suite (`atest.c` with 9 attacks), and Layer-2 kernel defense demonstration (`demo_kernel.py`).
- **Member B**: Process taint lifecycle in `struct proc`, allocation/free/fork zeroing and inheritance, syscall infrastructure (SYS 23, 24, 25), `sys_hello`, `sys_istainted`, user tools (`hello.c`, `mark.c`), test harness (`tlib.h`), `fileread` taint latch, `filewrite` write-down gate, test cases T0, T1, T3, T4, T11a in `ttest.c`, and cross-member audit pass #1.
- **Member C**: Inode layout redesign (`NDIRECT 11`, 64-byte `struct dinode` assertion, `untrusted` field in `fs.h`/`file.h`), `mkfs.c` verification, inode persistence across `ilock`/`iupdate`/`iput`, real `sys_marktaint` with argument validation, born-untrusted rule in `create()`, `sys_open` write/truncation gate, `sys_unlink` and `sys_link` refusals, two-boot reboot persistence pre-check, filesystem test suite (`ftest.c` with T5, T9, T11b, T13, T16, T18, T19), and cross-member audit pass #1.
- **Member D**: Pipe invariant formalization (`logs/pipe_invariant.md`), defs.h declarations, fail-closed stubs, pipe high-watermark tracking (`tainted_hi`, `has_taint`, per-byte updates), directory containment (`in_safe` with `.`/`..` skipping and self-match rejection), `ip_untrusted`, `kexec` pre-lock and post-lock containment in `exec.c`, pipe and exec test suite (`ptest.c` with T6, T7, T8, T10, T12, T14, T20, and D-09 concurrent stress test), and cross-member audit pass #1.

---

## 2. Authoritative Task Inventory & Completion Status

| Day | Task ID | Owner | Description | State | Acceptance / Test Evidence |
|---|---|---|---|---|---|
| 1 | X-01 | Member A | Freeze architecture decisions DEC-1..DEC-9 | COMPLETE | `CHANGES.md`, `docs/DECISIONS.md` |
| 1 | A-01 | Member A | Toolchain probe, baseline build/boot & usertests | COMPLETE | `docs/BASELINE_AUDIT.md`, `logs/G1/*.log` |
| 1 | A-02 | Member A | Evidence header generator & post-merge checker | COMPLETE | `scripts/evidence_header.sh`, `scripts/post_merge_check.sh` |
| 1 | A-04 | Member A | Compiling `atest.c` stub | COMPLETE | `user/atest.c` |
| 1 | B-01 | Member B | Repo layout, branches, tag baseline, templates | COMPLETE | `changes.md`, `PLAN_DEVIATIONS.md`, `logs/issues.md` |
| 1 | B-02 | Member B | `struct proc` tainted field & lifecycle | COMPLETE | `kernel/proc.h`, `kernel/proc.c` |
| 1 | B-04 | Member B | `tlib.h` test harness & `ttest.c` stub | COMPLETE | `user/tlib.h`, `user/ttest.c` |
| 1 | C-01 | Member C | Day-1 source verification & `layout.log` | COMPLETE | `logs/G1/layout.log` |
| 1 | C-02 | Member C | Layout: NDIRECT 11, uint untrusted, static assert | COMPLETE | `kernel/fs.h`, `kernel/file.h` |
| 1 | C-03 | Member C | Stubs: `sys_marktaint`, `ftest.c` | COMPLETE | `kernel/sysfile.c`, `user/ftest.c` |
| 1 | D-01 | Member D | Pipe invariant note in `pipe_invariant.md` | COMPLETE | `logs/pipe_invariant.md` |
| 1 | D-02 | Member D | `defs.h` taint block, `taint.c` stubs, `ptest.c` stub | COMPLETE | `kernel/defs.h`, `kernel/taint.c`, `user/ptest.c` |
| 1 | B-03 | Member B | Syscall plumbing: SYS 23/24/25, hello, istainted, mark | COMPLETE | `kernel/syscall.*`, `sysproc.c`, `user/user.h`, `usys.pl` |
| 1 | A-03 | Member A | Makefile wiring: user stubs, taint.o, fs.h dep | COMPLETE | `Makefile` |
| 1 | X-02 | Member B | Gate G1: plumbing merge, hello smoke, tag m1-plumbing | COMPLETE | `logs/G1/hello.log`, tag `m1-plumbing` |
| 2 | A-05 | Member A | Tool validator, planner & 20 host unit tests | COMPLETE | `agent_lab/xv6agent/`, 20/20 PASS in `test_validator.py` |
| 2 | B-05 | Member B | `fileread` taint latch (>0 bytes read taints) | COMPLETE | `kernel/file.c` |
| 2 | B-06 | Member B | `filewrite` write-down gate to clean files | COMPLETE | `kernel/file.c` |
| 2 | B-07 | Member B | `ttest.c`: T0, T1, T4 tests, complete mark CLI | COMPLETE | `user/ttest.c`, `user/mark.c` |
| 2 | B-08 | Member B | `ttest.c`: T3 positive-read vs 0-byte read | COMPLETE | `user/ttest.c` |
| 2 | C-04 | Member C | Inode persistence: `ilock`/`iupdate`/`iput` copy untrusted | COMPLETE | `kernel/fs.c` |
| 2 | C-05 | Member C | Real `sys_marktaint` with argument validation | COMPLETE | `kernel/sysfile.c` |
| 2 | C-06 | Member C | `create()` born-untrusted rule | COMPLETE | `kernel/sysfile.c` |
| 2 | C-09 | Member C | Two-boot persistence pre-check (G2) | COMPLETE | `logs/G2/t2_manual.log` |
| 2 | D-03 | Member D | Pipe watermark: `tainted_hi`, `has_taint`, per-byte | COMPLETE | `kernel/pipe.c` |
| 2 | D-04 | Member D | Real `in_safe` & `ip_untrusted` in `taint.c` | COMPLETE | `kernel/taint.c` |
| 2 | D-06 | Member D | `ptest.c`: T6 pipe taint, T7 clean pipe | COMPLETE | `user/ptest.c` |
| 2 | X-03 | Member C | Gate G2: vertical-slice evidence check | COMPLETE | `logs/G2/gate_g2_summary.log` |
| 3 | A-06 | Member A | QEMU bridge core, sentinel framing & recovery | COMPLETE | `agent_lab/xv6agent/bridge.py` |
| 3 | A-07 | Member A | Persistent-disk mode with SHA-256 checksums | COMPLETE | `agent_lab/xv6agent/bridge.py` |
| 3 | A-08 | Member A | 10 bridge unit/integration tests in `test_bridge.py` | COMPLETE | `logs/G3/test_bridge.log` (10/10 PASS) |
| 3 | A-10 | Member A | `make_lab_image.sh` staging script | COMPLETE | `agent_lab/make_lab_image.sh` |
| 3 | B-09 | Member B | `ttest.c`: T11a write refusal on clean FD after taint | COMPLETE | `user/ttest.c` |
| 3 | B-10 | Member B | Audit pass #1 of D's enforcement sites | COMPLETE | `logs/audit.md` |
| 3 | C-07 | Member C | `sys_open` write gate before `itrunc` | COMPLETE | `kernel/sysfile.c` |
| 3 | C-08 | Member C | `sys_unlink` and `sys_link` refusals | COMPLETE | `kernel/sysfile.c` |
| 3 | C-10 | Member C | Filesystem tests T5, T9, T11b, T16, T18, T19 | COMPLETE | `user/ftest.c` |
| 3 | C-11 | Member C | T13 inode reuse validation using `fstat` | COMPLETE | `user/ftest.c` |
| 3 | C-12 | Member C | Audit pass #1 of B's `file.c` sites | COMPLETE | `logs/audit.md` |
| 3 | D-05 | Member D | `kexec` pre-lock `in_safe` & post-lock untrusted check | COMPLETE | `kernel/exec.c` |
| 3 | D-07 | Member D | `ptest.c`: T8 mixed writers, T14 dup | COMPLETE | `user/ptest.c` |
| 3 | D-08 | Member D | `ptest.c`: T10, T12, T20 exec containment | COMPLETE | `user/ptest.c` |
| 3 | D-09 | Member D | Concurrent two-writer pipe stress test | COMPLETE | `user/ptest.c` |
| 3 | D-10 | Member D | Audit pass #1 of C's `sysfile.c` sites | COMPLETE | `logs/audit.md` |
| 3 | A-09 | Member A | 8-field trace `agent.py` & `demo_agent.py` | COMPLETE | `logs/G3/demo_agent_trace.json` |
| 3 | X-04 | Member D | Gate G3: initial enforcement check, tag m2-vertical | COMPLETE | `logs/G3/gate_g3_summary.log`, tag `m2-vertical` |
| 4 | A-11 | Member A | Red-team attack suite (A1..A4, A8, A9, A16..A18) | COMPLETE | `user/atest.c` |
| 4 | A-12 | Member A | INT1--INT7 integration test run | COMPLETE | `logs/G4/int1_int7_results.log` |
| 4 | A-13 | Member A | `demo_kernel.py` Runs A, B, C | COMPLETE | `logs/G4/demo_kernel.log` |
| 4 | B-11 | Member B | `ttest` runs and triage at CPUS=1 | COMPLETE | `logs/G4/ttest_cpus1.log` (5/5 PASS) |
| 4 | C-13 | Member C | `ftest` runs, T2 and T13 evidence | COMPLETE | `logs/G4/ftest_cpus1.log` (9/9 PASS) |
| 4 | D-11 | Member D | `ptest` runs and 10-trial stress evidence | COMPLETE | `logs/G4/ptest_cpus1.log` (8/8 PASS) |
| 4 | X-05 | Member A | Gate G4: agentic integration milestone, tag m3-ai-ready | COMPLETE | `logs/G4/gate_g4_summary.log`, tag `m3-ai-ready` |

---

## 3. Security Quality & Invariants Audit

1. **Inode Layout (Invariant I1)**:
   - `NDIRECT` set to 11.
   - `struct dinode` includes `uint untrusted` and compiles to exactly 64 bytes (`_Static_assert` passes).
   - `mkfs.c` compiles cleanly with host GCC; zeroes dinodes on allocation.
2. **Refusal Canonical Strings (Invariant I4)**:
   - All kernel denial paths strictly emit one of the four canonical strings:
     - `taint: unlink refused for pid %d\n`
     - `taint: link refused for pid %d\n`
     - `taint: write refused for pid %d\n`
     - `taint: exec refused for pid %d\n`
3. **Locking & Transaction Obligations**:
   - `sys_open`: Denied open-for-write releases `iunlockput(ip)` and `end_op()` before returning `-1`, preceding `itrunc(ip)` so clean files are never corrupted.
   - `exec.c`: `in_safe()` executes *before* `ilock(ip)` to prevent lock inversion/self-deadlock with `/safe`. On denial, unlocked `ip` is freed via `iput(ip)` and `end_op()`; locked `ip` is freed via `iunlockput(ip)` and `end_op()`.
   - `sys_unlink` and `sys_link`: Denials execute *before* `begin_op()`, avoiding log space exhaustion.
   - `filewrite`: Denials call `iunlock(f->ip)` and `end_op()` before returning `-1`.
4. **Watermark Pipe Safety**:
   - Monotonic high-water mark (`tainted_hi`) prevents false negatives. A clean reader reading past `tainted_hi` remains clean.

---

## 4. Verification Evidence & Artifact Summary

- **Exported Patches**: 47 patches stored in `artifacts/day1-day4-commits/0001-*.patch` through `0047-*.patch`.
- **Commit Manifest**: `artifacts/day1-day4-commits/COMMIT_MANIFEST.md`.
- **Replication Guide**: `artifacts/day1-day4-commits/REPLICATION_GUIDE.md`.
- **Unit & Integration Logs**:
  - `logs/G2/test_validator.log`: 20/20 host validator tests PASS.
  - `logs/G3/test_bridge.log`: 10/10 QEMU bridge tests PASS.
  - `logs/G3/demo_agent_trace.json`: Frozen 8-field trace showing prompt injection rejection at Layer 1.
  - `logs/G4/int1_int7_results.log`: INT1–INT7 integration suite PASS.
  - `logs/G4/demo_kernel.log`: Layer-2 kernel defense demonstration PASS (Runs A, B, C).
  - `logs/G4/ttest_cpus1.log`: 5/5 ttest PASS.
  - `logs/G4/ftest_cpus1.log`: 9/9 ftest PASS.
  - `logs/G4/ptest_cpus1.log`: 8/8 ptest PASS.
- **Git Tags**:
  - `baseline` (`15052a1`)
  - `m1-plumbing` (`0dcccbf`)
  - `m2-vertical` (`ddf25c3`)
  - `m3-ai-ready` (`68f5305`)
