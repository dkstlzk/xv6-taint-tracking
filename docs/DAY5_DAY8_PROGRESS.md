# Days 5–8 execution record

The four-member execution plan's detailed daily schedule is authoritative.
Task ownership uses its A/B/C/D labels; commits use the user-authorized identity
`pmakde <parneeka61@gmail.com>`. These are assisted implementation commits, not
claims that four different people authored, ran, or signed off the work.

## Day 5

- B-11: repaired child-result aggregation and duplicate includes; ttest runs on
  CPUS=3. Reports travel through xv6's 32-bit exit status, avoiding a tainted
  reporting pipe contaminating the suite coordinator.
- A-14/A-16/A-17: real private-disk QEMU bridge, nine attacks matching the named
  handbook scenarios, FP-1 through FP-5, strict unattended suite runner, and
  real transport tests. The old bridge and kernel demo were simulations, so
  historical G3/G4 claims are not sufficient for the new gates.
- C-14: token-bearing T2/T17 phases, clean-observer T5, eight-allocation T13,
  invalid-pointer validation, and OH2 regression tests.
- D-12: real allowed exec/taint-survival tests, failed/zero pipe reads, and
  20 seeded CPUS=3 trials with two 1024-byte writers, concurrent reader, and
  complete observed C/T byte orderings.
- Open defect: OH2 non-regular creation is missing. Both mkdir and mknod fail
  the new test. `logs/G5/gdb_oh2_before.log` records a real GDB breakpoint showing
  `type=1` and `proc->tainted=1` entering create, followed by the failing test.
  C owns the Day 6 fix (X-07/C-15).

Evidence lives under `logs/G5/day5/`; initial diagnostic runs under `logs/G5/`
may deliberately fail and are not gate passes. Each log names its source commit;
`SOURCE_DIRTY` discloses whether tracked implementation changes were present.

## Source-verified adjustments

- `user/sh.c` uses a 100-byte buffer, not the assumed 128. An 80-byte command plus
  compact random sentinel framing and newline fits the real buffer. Matching
  excludes the echoed `echo SENTINEL` command and tolerates no-newline file data.
- The old UPROGS continuation contained comments that dropped project binaries.
  STOCK_UPROGS and PROJECT_UPROGS now provide explicit images for both campaigns.
- Attack IDs A2/A3/A9/A16/A17/A18 were mismatched in the baseline; they now follow
  the handbook table. Undefined A5–A7/A10–A15 remain undefined.
- FP-5 feeds a marked file followed by the user's ls/rm commands to the same sh
  through a pipe. This exercises permanent shell taint with repeatable input;
  it is not a claim of an interactive terminal session.
- G5 is Day 7. The detailed schedule puts final FP classification/audit and G6
  on Day 9; this task stops after Day 8 and does not award G6 early.

## Day 6

- C-15/X-07: OH2 guard added before path lookup/allocation. ftest passes at
  CPUS=1/3, including the previously failing mkdir/mknod tests.
- A-15: T2/T17 pass across real reboots at both CPU counts with matching image
  paths/checksums and random tokens. Fresh-disk T2 correctly fails as a negative
  control. No rebuild occurs between either phase.
- D-12: 60 additional CPUS=3 trials pass. Watermark retained with measured limits
  and a checked, unmerged per-pipe fallback patch.
- X-06: denial audit #2 records all seven obligations and assigned review roles;
  no human signatures have been invented.
- A integration repair: INT1–INT8 and real Runs A/B/C replace simulated baseline
  evidence before G5. Run A builds the exact pinned upstream source from the
  existing local checkout; the project's initial import is not pristine stock.

Commits remain on the existing main branch in ordered member/task blocks, as in
the user's Day 1–4 history. This single-implementer execution does not fabricate
four feature-branch histories or peer approvals. Team review remains separate.

## Day 8

- A-18/B-12 (assigned to A): a fresh clone at g5-mandatory passes three clean
  suite runs at CPUS=1/3. The subsequent CPUS=3 integration run exposes ISS-010,
  a late shell prompt splitting the echoed command. Commit `21fdf18` drains the
  prompt before dispatch. The failure remains in `logs/G6/clean_clone/`.
- A-18/A-19: a second fresh clone at `880b60b` passes three clean builds and
  complete suite runs at each CPU count, 35 host tests per run, FP-1 through
  FP-5 at both CPU counts, and INT1–INT8/INT10/INT11. See
  `logs/G6/final_clone/` and `docs/TWO_LAYER_EVIDENCE.md`.
- B-12 (A): full stock usertests (quick and slow sections, stock-UPROGS image
  with patched kernel) pass at CPUS=1 and 3. These logs name `06a3c9a`; the
  kernel, stock user binaries and build rules remain identical at `880b60b`.
- C-17: optional abrupt-kill reboot passes at CPUS=1/3 after mark returns
  without an explicit sync. T2's token and same-image checks still apply.
  ftest is rerun at both CPU counts after adding that mode.
- D-13: two additional independent 60-trial CPUS=3 campaigns pass, one with a
  busy host CPU worker. Full byte orderings, seeds, totals and conditions are
  retained. Limits are recorded in `LIMITS.md`.
- D-13 final confirmation: 60 further CPUS=3 trials at seed 8103 pass with clean
  source provenance. This brings dedicated Days 5–8 stress evidence to 320 trials.
- A-21: individual/team/nested packaging and attribution scripts use immutable
  Git blobs and the true upstream baseline. A's individual and the team package
  at `880b60b` verify byte-for-byte; a tampered-byte control is rejected.

Remaining scheduled work: C-16 final FP classification/three-image persistence
repeats, X-09 final denial audit, Day 9 continuation of regression/stress and G6;
then Day 10–11 report, individual contribution review, viva/rehearsals and release.
Optional A-20 is not implemented. All four members' own review/signatures remain
human tasks. No g6-regression or release tag is created by this Day 8 work.
