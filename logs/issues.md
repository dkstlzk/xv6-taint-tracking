# Project Issue & Defect Tracking Log (logs/issues.md)

Maintained by Member B. Each row is owned and edited by the member responsible for that component.
Priority levels:
- **P0**: Blocks a milestone gate or build.
- **P1**: Defect to fix before Gate G6 (clean-build regression).
- **P2**: Documented limitation or non-blocking diagnostic finding.

| ID | Symptom | Reproduction command | Commit | Owning member | Priority | Status | Evidence of fix |
|---|---|---|---|---|---|---|---|
| ISS-001 | Host environment missing `riscv64-unknown-elf-gcc` / `gcc-riscv64-linux-gnu` cross-compiler and `qemu-system-riscv64` | `which riscv64-unknown-elf-gcc riscv64-linux-gnu-gcc qemu-system-riscv64` | `a269f85` | Member A / Env | P1 | OPEN (Documented limitation) | Toolchain verified in `docs/BASELINE_AUDIT.md`. Host toolchain must be installed to run guest builds locally. |
| ISS-002 | `evidence_header.sh` emitted shell error `UNKNOWN-gcc: command not found` when cross-compiler was absent | `./scripts/evidence_header.sh` | `a269f85` | Member A | P2 | CLOSED | Modified `scripts/evidence_header.sh` to gracefully detect missing compiler binary without shell syntax failure. |
| ISS-003 | `post_merge_check.sh` failed with exit code 3 when `external/xv6-riscv` was missing | `./scripts/post_merge_check.sh` | `a269f85` | Member A | P1 | CLOSED | Patched `post_merge_check.sh` to prioritize root `$PROJECT_ROOT/kernel` if present. |
| ISS-004 | Open instructor clarification questions U4–U10 (deadline, team parameters, LLM requirements, submission zip naming) | Review `CSL3030_Project3_v5.3.1_Final_QA.tex` §1.5 | `a269f85` | Member A / B | P2 | OPEN (Pending Instructor Response) | Consolidated question draft documented in `CHANGES.md` (DEC-9). |
