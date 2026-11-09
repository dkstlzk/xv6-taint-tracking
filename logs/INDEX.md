# Master Evidence Index (logs/INDEX.md)

**Authority**: CSL3030 Project 3 Four-Member Execution Plan (§16, Acceptance-Criteria Traceability Matrix) and Engineering Handbook Edition 5 (§25).  
**Purpose**: Map every matrix requirement row to its corresponding verified evidence file, status, owning member, and test identifiers.

---

## 1. Acceptance-Criteria Traceability Matrix & Evidence Mapping

| Req. ID | Required Behavior | Class | Owner | Test IDs | Evidence Artifact Path | Verified Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **BL1** | Baseline xv6-riscv builds and boots at pinned commit `f780d78`; stock usertests run | PLAN | A | boot, usertests | `logs/baseline_build.log`, `logs/baseline_usertests.log` | Demonstrated |
| **BL2** | Day-1 source verification of U1–U3 before any layout merge | PLAN | C | static assert, layout check | `logs/G1/layout.log` | Demonstrated |
| **BLD1** | Build integration: stubs in UPROGS, `taint.o` in OBJS, `fs.img` depends on `fs.h` | PLAN | A | clean build, boot | `logs/G1/build.log` | Demonstrated |
| **SYS-0**| `hello(a,b,c,d)` syscall prints `hello 1 2 3 4` in kernel, user prints `sum = 10` | SPEC | B | T0 | `logs/G1/hello.log`, `logs/G5/day7/ttest_cpus1.log` | Demonstrated |
| **R1a** | `untrusted` flag in `struct dinode`; layout and `mkfs` consistent (64 B, IPB 16) | SPEC | C | static assert, T2 | `logs/G1/layout.log`, `logs/G5/day7/t2_cpus1.log` | Demonstrated |
| **R1b** | `marktaint(path)`: 0 on regular file; -1 on missing, directory, empty, bad pointer | SPEC | C | T16, T2, T3 | `logs/G5/day7/ftest_cpus1.log`, `logs/G6/final_clone/run1/ftest_cpus1.log` | Demonstrated |
| **R2** | Reading >0 bytes from untrusted inode taints process; 0-byte read leaves clean | SPEC | B | T3, T12, A9 | `logs/G5/day7/ttest_cpus1.log`, `logs/G6/final_clone/run1/ttest_cpus1.log` | Demonstrated |
| **R3a** | Process taint inherited across `fork()` | SPEC | B | T4 | `logs/G5/day7/ttest_cpus1.log`, `logs/G6/final_clone/run1/ttest_cpus1.log` | Demonstrated |
| **R3b** | Files written by tainted process born untrusted; existing inode never re-labeled | SPEC | C | T5, A2 | `logs/G5/day7/ftest_cpus1.log`, `logs/G6/final_clone/run1/ftest_cpus1.log` | Demonstrated |
| **R3c** | Pipe propagation: watermark progression (`tainted_hi`), justified concurrency | SPEC | D | T6, T7, T8, T14, stress | `logs/pipe_invariant.md`, `logs/G6/stress_final_cpus3.log` | Demonstrated |
| **R4a** | Tainted process cannot unlink: -1, canonical log, victim file survives | SPEC | C | T9, A1, A3 | `logs/G5/day7/ftest_cpus1.log`, `logs/G6/final_clone/run1/atest_cpus1.log` | Demonstrated |
| **R4b** | Tainted process cannot exec outside `/safe` | SPEC | D | T10, A9 | `logs/G5/day7/ptest_cpus1.log`, `logs/G6/final_clone/run1/ptest_cpus1.log` | Demonstrated |
| **R4b'**| `exec("/safe")` and `exec("/")` rejected as directory targets (not inside `/safe`) | SPEC | D | T20 | `logs/G5/day7/ptest_cpus1.log`, `logs/G6/final_clone/run1/ptest_cpus1.log` | Demonstrated |
| **R4c** | Tainted process cannot write clean file (including open-for-write/truncate) | SPEC | C | T11a, T11b, T18, T19, A17, A18 | `logs/G5/day7/ttest_cpus1.log`, `ftest_cpus1.log` | Demonstrated |
| **R4d** | Refusal invariants: -1, exactly one canonical log line, all resources released | SPEC | B | all denial tests, audit table | `logs/audit.md` (Passes #1, #2, #3) | Demonstrated |
| **R5** | `istainted()` returns caller's taint status (0 clean, 1 tainted) | SPEC | B | T1, T3 | `logs/G5/day7/ttest_cpus1.log`, `logs/G6/final_clone/run1/ttest_cpus1.log` | Demonstrated |
| **HINT-1**| Taint survives `exec`; `p->tainted` never cleared on image replace | SPEC | D | T12, A9 | `logs/G5/day7/ptest_cpus1.log`, `logs/G6/final_clone/run1/atest_cpus1.log` | Demonstrated |
| **HINT-2**| Inode taint cleared when inode is freed and reused; confirmed by inode number | SPEC | C | T13 | `logs/G5/day7/ftest_cpus1.log`, `logs/G6/persistence_repeats/` | Demonstrated |
| **LC1** | Lifecycle across reboot: token persist (T2), unlinked absent, clean recreated (T17)| HB | C | T2, T17 | `logs/G5/day7/t2_cpus1.log`, `t17_cpus1.log`, `logs/G6/persistence_repeats/` | Demonstrated |
| **E1** | Demo chain: mark inbox, cat to sh, write-down denial, Runs A/B/C | SPEC | A | Runs A, B, C | `logs/G6/final_demo/demo_kernel_A_cpus1.log`, `B`, `C` | Demonstrated |
| **E2** | Laundering attack suite: at least 6 attacks blocked with mechanism named | SPEC | A | A1–A4, A8, A9, A16–A18 | `logs/G5/day7/atest_cpus1.log`, `logs/G6/final_clone/run1/atest_cpus1.log` | Demonstrated |
| **E3** | False-positive analysis table: benign vs intended policy vs bug (0 bugs) | SPEC | A/C | FP-1..FP-5 | `logs/G6/fp_classification.md`, `logs/G6/final_clone/fp_*` | Demonstrated |
| **H1** | Hidden category: laundering through file descriptor `dup()` | SPEC | D | T14 | `logs/G5/day7/ptest_cpus1.log`, `logs/G6/final_clone/run1/ptest_cpus1.log` | Demonstrated |
| **H2** | Hidden category: file written, unlinked, recreated | SPEC | C | T5, T9, T13, T17, A2 | `logs/G5/day7/ftest_cpus1.log`, `atest_cpus1.log` | Demonstrated |
| **H3** | Hidden category: pipe shared by concurrent tainted and clean writers | SPEC | D | T8, 320 stress trials | `logs/G6/stress_idle_cpus3.log`, `stress_load_cpus3.log` | Demonstrated |
| **AG1** | Python agent dispatcher and `validate_action` tool filtering | SPEC | A | `test_validator` (20 tests) | `agent_lab/tests/test_validator.py`, `logs/G6/final_clone/run1/host_tests.log` | Demonstrated |
| **AG2** | Working read-only tools (`read_file`, `list_directory`, etc.) | SPEC | A | `test_bridge` | `agent_lab/tests/test_bridge.py`, `logs/G6/final_clone/run1/host_tests.log` | Demonstrated |
| **AG3** | Documented QEMU bridge: sentinel completion, timeout recovery, persistent disk | SPEC | A | `test_bridge` (10 tests) | `agent_lab/xv6agent/bridge.py`, `agent_lab/tests/test_bridge.py` | Demonstrated |
| **AG4** | Repeatable prompt-injection defense in guest space | SPEC | A | `demo_agent`, INT4 | `logs/G4/demo_agent_run1.log`, `logs/G6/final_clone/integration_cpus1.log` | Demonstrated |
| **AG5** | Evidence of real kernel refusal (canonical log lines captured by bridge) | SPEC | A | Run C, INT5 | `logs/G6/final_demo/demo_kernel_C_cpus1.log`, `integration_cpus1.log` | Demonstrated |
| **AG6** | Per-run structured record (8-field trace JSON), controls, limits | SPEC | A | INT1–INT8, INT10, INT11 | `logs/G6/final_clone/agent_trace_cpus1.json`, `cpus3.json` | Demonstrated |
| **GATE**| Gates G1–G7 with cryptographic artifact manifests and commit headers | PLAN | B | G1, G2, G3, G4, G5, G6, G7 | `logs/G1/`, `G2/`, `G3/`, `G4/`, `G5/gate_g5.json`, `G6/gate_g6.json` | Demonstrated |
| **INT** | Full integration test suite INT1–INT11 through real `QemuBridge` | PLAN | A | INT1–INT11 | `logs/G6/final_clone/integration_cpus1.log`, `integration_cpus3.log` | Demonstrated |
| **V1** | Raw QEMU logs behind every claimed result, with commit and environment header | SPEC | A | all suites | `logs/G6/INDEX.md`, `logs/G5/day7/` | Demonstrated |
| **V2** | GDB session log of a real defect diagnosed and repaired | SPEC | C/A | OH2 non-regular create bug | `logs/G5/gdb_oh2_before.log`, `logs/G5/day6/ftest_cpus1.log` | Demonstrated |
| **V3** | Mock viva drills and live code-change demonstrations | SPEC | B | drills | `submissions/mock_viva_drills.md` | Demonstrated |
| **V4** | Design journal and commit trail with note-bearing task references | SPEC | B | git commit history | `logs/issues.md`, git log | Demonstrated |
| **V5** | Documented limits from measured results under real host/guest conditions | SPEC | B | measured thresholds | `LIMITS.md` | Demonstrated |
| **D1** | Clean packaging: only modified/added files, per-person highlights | SPEC | A | packaging verification | `scripts/packaging.py`, `submissions/` | Demonstrated |
| **D2** | Concise two-page report with mathematical watermark invariants | SPEC | D | report draft | `docs/REPORT.md` | Demonstrated |
| **PR1** | Individual submission packages for Lab-9 requirements | LAB9 | A | `verify_individual.sh` | `submissions/member_A.zip` through `member_D.zip` | Demonstrated |
| **PR2** | Identifiable per-member contributions in shared files | LAB9 | A | attribution reports | `CONTRIBUTIONS.md`, `changes.md` | Demonstrated |
| **PR3** | Individual assessability across all four subsystem roles | LAB9 | B | viva drills & checklists | `submissions/` | Demonstrated |
| **PR4** | Architectural and plan deviations frozen and documented | LAB9 | B | deviation log | `PLAN_DEVIATIONS.md`, `CHANGES.md` | Demonstrated |
| **RG1** | Three consecutive clean-build runs per CPU setting, stock usertests clean | PLAN | B | clean clone regression | `logs/G6/final_clone/run1/`, `run2/`, `run3/`, `usertests_cpus*.log` | Demonstrated |
| **DEMO1**| Final layered defense demonstration rehearsed incl. negative controls | PLAN | B | Runs A, B, C | `logs/G6/final_demo/` | Demonstrated |
| **RV1** | Abrupt-kill reboot persistence without explicit sync | OPT | C | kill-reboot | `logs/G6/abrupt/killreboot_cpus1.log`, `cpus3.log` | Demonstrated |
| **RV2** | Full stock usertests on stock-UPROGS image with patched kernel | OPT | B | stock usertests | `logs/G6/usertests_cpus1.log`, `usertests_cpus3.log` | Demonstrated |
| **RV3** | Multiprocessor runs on CPUS=3 across all guest suites | OPT | B | suites at CPUS=3 | `logs/G6/final_clone/run1/*_cpus3.log` | Demonstrated |
| **OH1** | Hardening: tainted `link()` refused (blocks attack A4) | OH | C | A4, T9 | `logs/G5/day7/atest_cpus1.log`, `logs/G6/final_clone/run1/atest_cpus1.log` | Demonstrated |
| **OH2** | Hardening: tainted non-regular file creation refused before inode allocation | OH | C | `ftest oh2` | `logs/G5/day7/ftest_cpus1.log`, `logs/G6/final_clone/run1/ftest_cpus1.log` | Demonstrated |
| **OH3** | Hardening: untrusted ELF binary inside `/safe` refused execution | OH | D | A16, T12 | `logs/G5/day7/atest_cpus1.log`, `logs/G6/final_clone/run1/atest_cpus1.log` | Demonstrated |
| **OPT1**| Simulated & deterministic LLM planner with frozen schema | OPT | A | validator suite | `agent_lab/xv6agent/planner.py`, `agent_lab/tests/test_validator.py` | Demonstrated |

---

## 2. Evidence Verification Summary
- **Total Requirements Mapped**: 53 / 53.
- **Failures / Regressions**: 0.
- **Inconclusive Items**: 0.
- **Cryptographic Gate Hashes**: Recorded in `logs/G5/day7/gate_g5.json` and `logs/G6/gate_g6.json`.

