# Day 8–9 Milestone M7 evidence index (Gate G6)

The detailed execution schedule assigns Milestone M7 regression and security validation to Days 8–9, concluding with Gate G6 (X-10). All claimed runs have raw transcripts, commit headers, and cryptographic checksums in `gate_g6.json`.

| Task / owner | Evidence | Result |
|---|---|---|
| A-18 / A | `clean_clone/run1/`, `run2/`, `run3/` | Original g5 clone: all guest suites and host tests pass at both CPU settings |
| A-19 / A | `clean_clone/integration_cpus3.log` | Historical FAIL: prompt race, fixed in 21fdf18; retained for audit transparency |
| A-18/B-12 / A | `final_clone/run1/`, `run2/`, `run3/` | Final 880b60b clone: three consecutive clean runs, all suites CPUS=1/3; 35 host tests per run |
| A-16/A-18 / A | `final_clone/fp_1_cpus1.log` through `fp_5_cpus3.log` | All five raw controls pass at both CPU counts |
| C-16 / C | `fp_classification.md`, `persistence_repeats/` | Complete classification of FP-1..FP-5 (zero bugs); T2, T17, and T13 REUSE-CONFIRMED repeated on 3 fresh disk images |
| A-19 / A | `final_clone/integration_cpus1.log`, `integration_cpus3.log`, associated agent_trace JSON | INT1–INT8, INT10 and INT11 pass; validator and kernel evidence kept distinct |
| B-12 / A | `usertests_cpus1.log`, `usertests_cpus3.log` | Full stock-UPROGS usertests on patched kernel: ALL TESTS PASSED |
| C-17 / C (host runner A) | `abrupt/killreboot_cpus1.log`, `killreboot_cpus3.log` | Same image/token passes after SIGKILL without explicit sync |
| C-17 regression / C | `abrupt/ftest_cpus1.log`, `ftest_cpus3.log` | All ten filesystem checks pass after new optional phase |
| D-13 / D | `stress_idle_cpus3.log`, `stress_load_cpus3.log` | 60 trials each, 1024 C + 1024 T bytes/trial, zero false negatives observed; latter has one busy host CPU worker |
| D-13 confirmation / D | `stress_final_cpus3.log` | Additional 60 trials with a clean source tree; earlier idle log transparently records concurrent unrelated source edits |
| A-21 / A | `packaging.log` | Individual/team canonical byte verification passes; tampered archive rejected |
| A-19 demo rerun / A | `final_demo/demo_kernel_A_cpus1.log`, B and C logs | Stock/unmarked deletion controls and marked-input protection repeated on final implementation |
| X-09 / C | `../audit.md` (Pass #3) | Final ring audit on all 10 denial sites across all 7 obligations; 0 leaks/defects |
| X-10 / B | `gate_g6.json` | Gate G6 passed with verified evidence across 61 test logs; tag `g6-regression` |

`prompt_fix/` is a diagnostic confirmation made before the final clean clone;
the final clone is the authoritative post-fix campaign. `../G5/day7/gate_g5.json`
contains the G5 artifact hashes, including T2/T17 and fresh-image negative
controls. `../G5/gdb_oh2_before.log` records the real X-07 bug diagnosis, and
Day 6/7 ftest logs demonstrate its fix.
