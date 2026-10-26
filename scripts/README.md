# Member A Scripts: Usage Documentation

This directory contains automation and validation scripts developed by Member A for the xv6 RISC-V Taint-Tracking project (CSL3030, IIT Jodhpur).

---

## 1. `evidence_header.sh`

### Purpose
Emits a standardized evidence header required for all project logs and test transcripts according to Engineering Handbook §503–504. The header captures environment provenance to ensure reproducibility across team members.

### Standard Format Output
```text
================================================================
EVIDENCE HEADER
================================================================
COMMIT:       <git commit SHA>
DATE:         <ISO-8601 timestamp>
CPUS:         <configured CPU count, default: not set (default 3)>
GCC_VERSION:  <compiler binary and version>
QEMU_VERSION: <QEMU emulator version>
COMMAND:      <command invocation>
================================================================
```

### Usage
```bash
# Print header to standard output
scripts/evidence_header.sh

# Record command context into a log file
scripts/evidence_header.sh "make clean && make fs.img" | tee -a logs/G1/build.log

# Override CPU count
CPUS=1 scripts/evidence_header.sh "usertests" | tee -a logs/G1/usertests.log
```

### Exit Codes
- `0`: Header generated successfully.
- `1`: Error (e.g., git repository cannot be resolved).

---

## 2. `post_merge_check.sh`

### Purpose
Implements the plan's mandatory post-merge verification gate (Handbook §405, §496):
> "After each merge `make clean && make qemu` must boot. Member A's `post_merge_check.sh` verifies each merge."

### Workflow
1. Emits standard evidence header using `evidence_header.sh`.
2. Executes `make clean && make` (kernel build verification).
3. Executes `make fs.img` (user programs and file system image build).
4. Boots xv6 in QEMU non-interactively with a timeout.
5. Verifies that `init: starting sh` is printed by the xv6 kernel.
6. Scans boot output for kernel panics.
7. Reports clear PASS/FAIL status.

### Usage
```bash
# Full check (build kernel + fs.img + boot in QEMU)
scripts/post_merge_check.sh

# Run with single CPU (CPUS=1)
CPUS=1 scripts/post_merge_check.sh

# Build-only mode (skip QEMU boot test)
scripts/post_merge_check.sh --skip-boot

# Custom xv6 directory (if different from default external/xv6-riscv)
XV6_DIR=/path/to/xv6-riscv scripts/post_merge_check.sh
```

### Exit Codes
- `0`: All checks passed (or `--skip-boot` passed clean build).
- `1`: Build failure (`make clean`, `make`, or `make fs.img`).
- `2`: Boot failure (QEMU did not print `init: starting sh` or panicked).
- `3`: Usage or configuration error (invalid flags or missing directories).
