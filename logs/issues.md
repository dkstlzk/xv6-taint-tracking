# Project Issue & Defect Tracking Log (logs/issues.md)

Maintained by Member B. Each row is owned and edited by the member responsible for that component.
Priority levels:
- **P0**: Blocks a milestone gate or build.
- **P1**: Defect to fix before Gate G6 (clean-build regression).
- **P2**: Documented limitation or non-blocking diagnostic finding.

| ID | Symptom | Reproduction command | Commit | Owning member | Priority | Status | Evidence of fix |
|---|---|---|---|---|---|---|---|
| ISS-001 | Host environment missing compiler and QEMU | `command -v riscv64-linux-gnu-gcc qemu-system-riscv64` | `d64c43f` | Member A / Env | P1 | CLOSED | Toolchain installed; real CPUS=1/3 build and suite logs in `logs/G5/day6/`. |
| ISS-002 | `evidence_header.sh` emitted shell error `UNKNOWN-gcc: command not found` when cross-compiler was absent | `./scripts/evidence_header.sh` | `a269f85` | Member A | P2 | CLOSED | Modified `scripts/evidence_header.sh` to gracefully detect missing compiler binary without shell syntax failure. |
| ISS-003 | `post_merge_check.sh` failed with exit code 3 when `external/xv6-riscv` was missing | `./scripts/post_merge_check.sh` | `a269f85` | Member A | P1 | CLOSED | Patched `post_merge_check.sh` to prioritize root `$PROJECT_ROOT/kernel` if present. |
| ISS-004 | Open instructor clarification questions U4–U10 (deadline, team parameters, LLM requirements, submission zip naming) | Review `CSL3030_Project3_v5.3.1_Final_QA.tex` §1.5 | `a269f85` | Member A / B | P2 | OPEN (Pending Instructor Response) | Consolidated question draft documented in `CHANGES.md` (DEC-9). |
| ISS-005 | Fake bridge/demo results and empty child summaries | `bash scripts/run_all.sh` | `1395b7e`, `3d1d451`, `bf597e0` | A/B | P0 | CLOSED | Real bridge tests, strict summary accounting, measured demos replace simulations. |
| ISS-006 | Non-regular creation allowed after taint | `ftest oh2` | `fd3ac9d` | C | P0 | CLOSED | `logs/G5/gdb_oh2_before.log`; both OH2 cases pass in Day 6 ftest logs. |
| ISS-007 | Attack IDs did not exercise handbook scenarios; exec test never execed | `atest`, `ptest` | `2e66115`, `244fe05` | A/D | P0 | CLOSED | Nine real attacks, valid ELF controls, and allowed-exec taint checks in Day 6 logs. |
| ISS-008 | Framing overflow and no-newline reads timeout | host bridge tests | `f4321ea`, `6a17a2f` | A | P0 | CLOSED | 80-byte and no-newline regression tests in host suite. |
| ISS-009 | Initial import called stock but already includes process-taint edits | compare initial import with pinned source | `15052a1` | A | P2 | DOCUMENTED | Run A uses exact upstream `f780d7850765d74cbdcd1fff4b822d8829f7c9bd` exported from local xv6 checkout, not the initial project import. |
