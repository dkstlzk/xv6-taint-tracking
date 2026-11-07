# Member A Scripts: Usage Documentation

## Days 5–8 reproducible commands

Run from the repository root with Python 3, make, the RISC-V GNU toolchain,
QEMU RISC-V (7.2+), and bc installed. No external Python packages are needed.

```bash
make -j4 kernel/kernel fs.img
LOG_DIR=logs/new-run bash scripts/run_all.sh
bash agent_lab/make_lab_image.sh
python3 -m scripts.fp_runner --log-dir logs/new-run
python3 -m scripts.persistence --log-dir logs/new-run --cpus 3
python3 -m scripts.integration --layering --log-dir logs/new-run --cpus 3
python3 -m scripts.stress_campaign --trials 60 --seed 8102 --load --log logs/new-run/stress.log
python3 -m scripts.stock_usertests --cpus 3 --log-dir logs/new-run
python3 -m scripts.regression --ref HEAD --runs 3 --output logs/new-clone
```

Use a new log/output directory to preserve earlier evidence. `run_all.sh`
builds both kernel and project image, runs the real host/bridge tests, then
ttest/ftest/ptest/atest on both CPU settings. `CPU_LIST=1` restricts a diagnostic
run. Missing/mismatched summaries, failure or inconclusive results, panic,
transport error, and timeout cause nonzero exit; suite failure transcripts are
also copied into `failed/`. Guest suite counts are checked against result lines.
QEMU completion is not a claim about an arbitrary command's exit code.

Each QemuBridge uses a private image; fs.img and lab.img are templates. Persistent
restart retains the private image and records its checksum before boot 2; fresh
restart copies the original template again. The fixed framing fits this tree's
100-byte shell buffer and consumes the following prompt. Trusted fixtures alone
enable shell operators; agent actions pass through the validator first.

`python3 -m scripts.persistence --abrupt --cpus 3 --log-dir logs/new-run` runs
optional C-17: SIGKILL QEMU after the mark phase returns without an explicit sync,
then boots the same image. It observes this timing window, not every crash point
or physical power-loss behavior. Normal T2/T17 phases explicitly sync.

For the stock demo, build an untouched export of `upstream-pinned` separately:

```bash
mkdir -p /tmp/xv6-demo-stock
git archive upstream-pinned | tar -x -C /tmp/xv6-demo-stock
make -C /tmp/xv6-demo-stock -j4 kernel/kernel fs.img
python3 -m agent_lab.demo_kernel --baseline-root /tmp/xv6-demo-stock --log-dir logs/new-run
```

Run A uses that stock kernel; B uses an unmarked fixture on the patched kernel;
C marks the fixture and verifies the real refusal and surviving file. The
initial project import already has process-taint edits and is not stock.

### Preliminary packaging (A-21)

```bash
bash scripts/pack_individual.sh A g5-mandatory
bash scripts/verify_individual.sh artifacts/packages/member_A.zip g5-mandatory
bash scripts/attribution_report.sh g5-mandatory
LAYOUT=both bash scripts/pack_individual.sh A HEAD
```

`LAYOUT=individual|team|both|nested` selects outputs under `artifacts/packages/`
(ignored generated files). Individual ZIPs contain canonical `files/`, a hash
manifest, and per-file commit notes. Team ZIPs include files added/modified from
the exact pinned upstream tree plus an explicit deleted-path list. Overlay
`files/` on that tree and honor the deleted list. Verify compares file membership
and every byte against the requested commit; it rejects extras and duplicates.
The nested layout contains the selected individual ZIP and team ZIP.

Assignment derives from `[Member A/B/C/D]` commit subjects because the user chose
one real author identity for this execution. It does not certify personal
authorship or resolve unattributed historical imports. The Day 10 team review,
individual manifests/signatures, final zip naming, and release packaging remain
separate tasks. Keep the `upstream-pinned` tag when cloning for packaging.

G5's machine-readable evidence checklist is `python3 -m scripts.check_g5`.
Its default input is the immutable Day 7 campaign. Day 9's final audit and G6
sign-off are intentionally pending even when Day 8 regression runs are green.

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
