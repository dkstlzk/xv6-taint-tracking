# Day 1–Day 4 Implementation and Repository Audit

This audit evaluates the state of the `os-project` repository against the authoritative four-member plan (*CSL3030 Project 3 Four-Member Execution Plan*, *CSL3030 Project 3 Final QA Specification v5.3.1*, and *xv6-riscv Engineering Handbook Edition 5*).

---

## 1. Environment & Toolchain Audit

An audit of host tools was conducted:
- **Host GCC**: `/usr/bin/gcc` (GCC 13.2.0) — Available.
- **GNU Make**: `/usr/bin/make` (GNU Make 4.4.1) — Available.
- **Python 3**: `/home/dkstlzk/.pyenv/shims/python3` (Python 3.12.3) — Available.
- **RISC-V Cross Compiler (`riscv64-unknown-elf-gcc` / `riscv64-linux-gnu-gcc`)**: **NOT INSTALLED** on host PATH. Sudo privileges are restricted.
- **QEMU RISC-V System Emulator (`qemu-system-riscv64`)**: **NOT INSTALLED** on host PATH.

> [!WARNING]
> Historical logs in `logs/G1/` (`build.log`, `layout.log`, `baseline_usertests_cpus1.log`) were recorded from an earlier execution environment possessing the complete RISC-V cross-compiler and QEMU. On the current host, dynamic kernel execution is bounded by toolchain availability; therefore, strict static analysis, compile verification of host components (`agent_lab`), syntax audits, and exact file/structure compliance are enforced, while runtime guest execution is accurately documented.

---

## 2. Task-by-Task Implementation Status (Days 1–4)

| Day | Task ID | Owner | Required Behavior | Existing State | Missing Work | Acceptance Test | Status |
|---|---|---|---|---|---|---|---|
| 1 | X-01 | Member A | Freeze architecture decisions DEC-1..DEC-9 in CHANGES.md; prepare instructor questions | `CHANGES.md` and `docs/DECISIONS.md` drafted | Review and finalize all DEC-1..DEC-9 against QA specification | All 9 decisions documented and confirmed/ratified | COMPLETE |
| 1 | A-01 | Member A | Toolchain verification, baseline build/boot, stock usertests, save logs | `docs/BASELINE_AUDIT.md` and `logs/G1/baseline_*.log` present | None (historical evidence preserved, host limits documented) | Baseline logs exist with valid header | COMPLETE |
| 1 | A-02 | Member A | Evidence-header script and `post_merge_check.sh` | Present in `scripts/` | Ensure correct execution and error handling | Script executes cleanly and exits non-zero on failure | COMPLETE |
| 1 | A-04 | Member A | Compiling `atest.c` stub | `user/atest.c` exists (exits 0) | None | Compiles under UPROGS | COMPLETE |
| 1 | B-01 | Member B | Repo layout, branches, tag baseline, templates, git identities, issue log | `logs/issues.md`, `changes.md`, `PLAN_DEVIATIONS.md` present; no git commits yet | Create clean initial baseline and templates | Git branches, tag `baseline`, templates committed | COMPLETE |
| 1 | B-02 | Member B | `struct proc` tainted field and allocation/free/fork lifecycle | Present in `kernel/proc.h` and `kernel/proc.c` | Verify lifecycle zeroing and fork copy | Lifecycle zeroing verified | COMPLETE |
| 1 | B-04 | Member B | `tlib.h` test harness and `ttest.c` stub | Present in `user/tlib.h` and `user/ttest.c` | Ensure exit code and macros match spec | Harness formats match spec | COMPLETE |
| 1 | C-01 | Member C | Day-1 source verification and `layout.log` | `logs/G1/layout.log` exists | None | Log confirms 64B dinode, NDIRECT 12 baseline | COMPLETE |
| 1 | C-02 | Member C | Layout: NDIRECT 11, uint untrusted in dinode/inode, static assert | Inode layout not yet modified in `fs.h`/`file.h` | Update `fs.h`, `file.h`, adjust bmap/itrunc | `_Static_assert(sizeof(struct dinode) == 64)` holds | NOT STARTED |
| 1 | C-03 | Member C | Stubs: `sys_marktaint` (returns 0), `ftest.c` | Not present | Add stub in `sysfile.c`, create `user/ftest.c` | Compiles under UPROGS | NOT STARTED |
| 1 | D-01 | Member D | Pipe invariant note in `logs/pipe_invariant.md` | Not present | Document pipe watermark invariant and limits | File `logs/pipe_invariant.md` exists | NOT STARTED |
| 1 | D-02 | Member D | `defs.h` taint block, `taint.c` fail-closed stubs, `ptest.c` stub | Not present | Add `kernel/taint.c`, update `defs.h`, add `ptest.c` | `taint.o` links, fail-closed stubs | NOT STARTED |
| 1 | B-03 | Member B | Syscall plumbing: SYS 23/24/25 (`sys_hello`, `sys_istainted`, `sys_marktaint`), `hello.c`, `mark.c` | Not present | Update `syscall.h`, `syscall.c`, `sysproc.c`, `usys.pl`, `user.h`, create `hello.c`, `mark.c` | Syscalls registered; user binaries exist | NOT STARTED |
| 1 | A-03 | Member A | Makefile wiring: user stubs, `taint.o`, `fs.h` dependency | Makefile has partial stock rules | Wire all six user programs, `taint.o`, dependency | `make fs.img` includes 6 stubs and `taint.o` | NOT STARTED |
| 1 | X-02 | Member B | Gate G1: merge plumbing, run hello, tag `m1-plumbing` | Historical logs exist in `logs/G1/` | Verify all Day 1 deliverables and tag | Gate G1 exit criteria met | NOT STARTED |
| 2 | A-05 | Member A | Validator, planner, and 19+ host unit tests in `agent_lab` | Not present | Create `agent_lab/xv6agent/{config,tools,planner}.py` and tests | 19+ unit tests pass with `python3` | NOT STARTED |
| 2 | B-05 | Member B | `fileread` taint latch for positive-byte reads from untrusted inode | Not present | Implement latch in `kernel/file.c` | Reading >0 bytes from untrusted taints process | NOT STARTED |
| 2 | B-06 | Member B | `filewrite` write-down gate (refuse write to clean regular file) | Not present | Implement gate in `kernel/file.c` logging canonical write refused line | Tainted process cannot write clean file; returns -1 | NOT STARTED |
| 2 | B-07 | Member B | `ttest.c`: T0, T1, T4 tests, complete `mark.c` CLI | `ttest.c` is stub | Implement T0, T1, T4 in `ttest.c`, CLI in `mark.c` | T0, T1, T4 pass; mark CLI functional | NOT STARTED |
| 2 | B-08 | Member B | `ttest.c`: T3 positive read vs 0-byte read test | Not present | Implement T3 in `ttest.c` | T3 verifies >0 byte read taints, 0-byte does not | NOT STARTED |
| 2 | C-04 | Member C | Inode persistence: `ilock`/`iupdate`/`iput` copy untrusted flag | Not present | Update `kernel/fs.c` persistence logic | Untrusted flag read/written to disk dinode | NOT STARTED |
| 2 | C-05 | Member C | Real `sys_marktaint` with argument validation | Not present | Implement `sys_marktaint` in `kernel/sysfile.c` | Marks regular file untrusted, rejects invalid args | NOT STARTED |
| 2 | C-06 | Member C | `create()` born-untrusted rule without re-labeling existing | Not present | Update `create()` in `kernel/sysfile.c` | New file created by tainted process is untrusted | NOT STARTED |
| 2 | C-09 | Member C | Manual reboot persistence pre-check (G2) | Not present | Record procedure and log in `logs/G2/` | Marked file remains untrusted across reboot | NOT STARTED |
| 2 | D-03 | Member D | `pipe.c` watermark: `tainted_hi`, `has_taint`, per-byte tracking | Not present | Add fields to `struct pipe`, update read/write logic | Per-byte watermark tracking in `kernel/pipe.c` | NOT STARTED |
| 2 | D-04 | Member D | Real `in_safe` and `ip_untrusted` in `kernel/taint.c` | Not present | Implement directory search for `/safe`, self-match rejection | Correctly checks direct children of `/safe` | NOT STARTED |
| 2 | D-06 | Member D | `ptest.c`: T6 tainted pipe, T7 clean pipe | `ptest.c` is stub | Implement T6 and T7 in `user/ptest.c` | T6 taints reader, T7 leaves reader clean | NOT STARTED |
| 2 | X-03 | Member C | Gate G2: core vertical slice evidence check | Not present | Audit Day 2 deliverables, collect logs | G2 evidence verified | NOT STARTED |
| 3 | A-06 | Member A | QEMU bridge core: sentinel framing, timeout, recovery | Not present | Create `agent_lab/xv6agent/bridge.py` | Bridge handles framing, timeout, restart | NOT STARTED |
| 3 | A-07 | Member A | Persistent-disk mode in QEMU bridge | Not present | Add `persist=True` support to `bridge.py` | Disk survives restart in persistent mode | NOT STARTED |
| 3 | A-08 | Member A | `test_bridge.py` with 9+ real/mock bridge tests | Not present | Implement unit/integration tests for bridge | 9+ bridge tests pass | NOT STARTED |
| 3 | A-10 | Member A | `make_lab_image.sh` staging script | Not present | Create `agent_lab/make_lab_image.sh` | Creates disk image with required lab files | NOT STARTED |
| 3 | B-09 | Member B | `ttest.c`: T11a (write via FD opened clean then tainted) | Not present | Implement T11a in `user/ttest.c` | Writing to clean FD after tainting returns -1 | NOT STARTED |
| 3 | B-10 | Member B | Audit pass #1 of D's sites into `logs/audit.md` | Not present | Audit `pipe.c`, `taint.c`, `exec.c` | 7 denial obligations checked and logged | NOT STARTED |
| 3 | C-07 | Member C | `sys_open` write gate before `itrunc` | Not present | Add check in `sys_open` (`kernel/sysfile.c`) | Tainted open for O_WRONLY/O_TRUNC on clean file fails | NOT STARTED |
| 3 | C-08 | Member C | `sys_unlink` and `sys_link` refusals | Not present | Add taint refusal in `sys_unlink` and `sys_link` | Tainted unlink/link returns -1 and logs | NOT STARTED |
| 3 | C-10 | Member C | `ftest.c`: T5, T9, T11b, T16, T18, T19 | Not present | Implement comprehensive FS test suite | All filesystem security tests pass | NOT STARTED |
| 3 | C-11 | Member C | `ftest.c`: T13 inode reuse with `fstat` check | Not present | Implement T13 verifying reused inode is clean | PASS or INCONCLUSIVE reported accurately | NOT STARTED |
| 3 | C-12 | Member C | Audit pass #1 of B's `file.c` sites into `logs/audit.md` | Not present | Audit `fileread` and `filewrite` denial paths | Lock/log/return paths audited | NOT STARTED |
| 3 | D-05 | Member D | `exec.c`: pre-lock `in_safe`, post-lock type/untrusted checks | Not present | Implement checks in `kexec` (`kernel/exec.c`) | Tainted exec outside `/safe` refused and logged | NOT STARTED |
| 3 | D-07 | Member D | `ptest.c`: T8 mixed writers, T14 dup, partial/long write | Not present | Implement T8, T14 in `user/ptest.c` | Watermark ordering and dup tested | NOT STARTED |
| 3 | D-08 | Member D | `ptest.c`: T10, T12, T20 (exec containment) | Not present | Implement T10, T12, T20 in `user/ptest.c` | `/safe` execution, taint survival, self-match rejection | NOT STARTED |
| 3 | D-09 | Member D | `ptest.c`: stress test two writers + reader | Not present | Implement concurrent stress test in `user/ptest.c` | Stress test executes without false negatives | NOT STARTED |
| 3 | D-10 | Member D | Audit pass #1 of C's `sysfile.c` sites into `logs/audit.md` | Not present | Audit `create`, `sys_open`, `sys_unlink`, `sys_link` | Obligations audited in `logs/audit.md` | NOT STARTED |
| 3 | A-09 | Member A | `agent.py` 8-field trace and `demo_agent.py` | Not present | Implement `agent.py` and `demo_agent.py` in `agent_lab` | 8-field trace JSON emitted; injection rejected | NOT STARTED |
| 3 | X-04 | Member D | Gate G3: initial enforcement verification, tag `m2-vertical` | Not present | Verify all G3 tests and audit tables | Gate G3 exit criteria met, tag created | NOT STARTED |
| 4 | A-11 | Member A | Attacks A1, A2, A3, A8, A9, A17, A18 in `atest.c` | `atest.c` is stub | Implement red-team attack suite in `user/atest.c` | Attacks report PASS with blocking mechanism | NOT STARTED |
| 4 | A-12 | Member A | G4 run: bridge tests, `demo_agent`, INT1-INT7 | Not present | Execute validation scripts against host harness | INT1-INT7 verified | NOT STARTED |
| 4 | A-13 | Member A | `demo_kernel.py` Runs A, B, C | Not present | Implement Layer-2 demo in `agent_lab/demo_kernel.py` | Runs A/B/C demonstrate kernel refusal | NOT STARTED |
| 4 | B-11 | Member B | `ttest` runs and triage in `logs/issues.md` | Not present | Run ttest suite, triage findings in `issues.md` | Issues triaged and logged | NOT STARTED |
| 4 | C-13 | Member C | `ftest` runs, T2 two-boot log, T13 evidence, fixes | Not present | Run ftest suite, document persistence evidence | FS test evidence recorded | NOT STARTED |
| 4 | D-11 | Member D | `ptest` runs, 10-trial stress, per-pipe fallback patch | Not present | Run ptest, generate fallback patch if needed | Pipe test evidence recorded | NOT STARTED |
| 4 | X-05 | Member A | Gate G4: agentic integration milestone, tag `m3-ai-ready` | Not present | Verify G4 deliverables, tag `m3-ai-ready` | Gate G4 complete, tag created | NOT STARTED |
