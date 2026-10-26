#!/usr/bin/env bash
# post_merge_check.sh — Verify that the xv6 tree builds and boots after a merge.
#
# This script is run by Member A (or any member) after every merge to main.
# It implements the plan's rule: "after each merge  make clean && make qemu
# must boot" (handbook §405, §496).
#
# Usage:
#   scripts/post_merge_check.sh                 # uses defaults
#   CPUS=1 scripts/post_merge_check.sh          # override CPU count
#   scripts/post_merge_check.sh --skip-boot     # build only, no QEMU
#
# What it does:
#   1. Prints the evidence header (commit, date, toolchain).
#   2. Runs  make clean && make  (kernel only — first target).
#   3. Runs  make fs.img  (user programs + filesystem image).
#   4. Boots xv6 under QEMU with a timeout, checks for "init: starting sh".
#   5. Prints PASS / FAIL summary with exit code.
#
# Exit codes:
#   0 — build succeeded and xv6 booted (or --skip-boot and build succeeded)
#   1 — build failed
#   2 — boot failed (QEMU did not produce "init: starting sh")
#   3 — usage error
#
# Requirements:
#   - RISC-V cross-compiler (detected automatically)
#   - qemu-system-riscv64
#   - GNU Make, bc, perl
#   - Must be run from the project root or with XV6_DIR set
#
# Plan references:
#   - Handbook §405: "A's post_merge_check.sh verifies each merge"
#   - Handbook §496: "After each merge  make clean && make qemu must boot"
#   - Plan: merge window 6-7pm daily Days 1-7; order C → D → B → A
#   - Evidence goes to logs/<gate>/
#
# Author: Member A
# ---------------------------------------------------------------

set -uo pipefail

# ---- Configuration -----------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
XV6_DIR="${XV6_DIR:-$PROJECT_ROOT/external/xv6-riscv}"
BOOT_TIMEOUT="${BOOT_TIMEOUT:-30}"
SKIP_BOOT=0
CPUS="${CPUS:-3}"
export CPUS

# ---- Parse arguments ---------------------------------------------
for arg in "$@"; do
    case "$arg" in
        --skip-boot) SKIP_BOOT=1 ;;
        --help|-h)
            head -30 "$0" | grep '^#' | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "ERROR: Unknown argument: $arg" >&2
            echo "Usage: $0 [--skip-boot] [--help]" >&2
            exit 3
            ;;
    esac
done

# ---- Helpers -----------------------------------------------------
log()  { echo "[post_merge_check] $*"; }
pass() { echo ""; echo "===== PASS: $* ====="; }
fail() { echo ""; echo "===== FAIL: $* ====="; }

# ---- Pre-flight checks ------------------------------------------
if [ ! -d "$XV6_DIR/kernel" ]; then
    echo "ERROR: xv6 source not found at $XV6_DIR" >&2
    echo "Set XV6_DIR to the xv6-riscv directory." >&2
    exit 3
fi

if [ ! -f "$XV6_DIR/Makefile" ]; then
    echo "ERROR: No Makefile in $XV6_DIR" >&2
    exit 3
fi

# ---- Evidence header ---------------------------------------------
log "Starting post-merge verification"
echo ""
if [ -x "$SCRIPT_DIR/evidence_header.sh" ]; then
    bash "$SCRIPT_DIR/evidence_header.sh" "post_merge_check.sh (CPUS=$CPUS)"
else
    echo "WARNING: evidence_header.sh not found; printing minimal header"
    echo "COMMIT: $(cd "$PROJECT_ROOT" && git rev-parse HEAD 2>/dev/null || echo 'unknown')"
    echo "DATE:   $(date --iso-8601=seconds 2>/dev/null || date)"
    echo "CPUS:   $CPUS"
fi
echo ""

# ---- Step 1: Clean build (kernel) --------------------------------
log "Step 1/4: make clean && make (kernel)"
cd "$XV6_DIR"

if ! make clean 2>&1; then
    fail "make clean failed"
    exit 1
fi

if ! make 2>&1; then
    fail "kernel build failed"
    exit 1
fi

if [ ! -f kernel/kernel ]; then
    fail "kernel/kernel not produced"
    exit 1
fi

pass "Kernel built successfully"
ls -la kernel/kernel

# ---- Step 2: Build fs.img (user programs + image) ----------------
log "Step 2/4: make fs.img"

if ! make fs.img 2>&1; then
    fail "make fs.img failed"
    exit 1
fi

if [ ! -f fs.img ]; then
    fail "fs.img not produced"
    exit 1
fi

pass "fs.img built successfully"
ls -la fs.img

# ---- Step 3: Boot check -----------------------------------------
if [ "$SKIP_BOOT" -eq 1 ]; then
    log "Step 3/4: SKIPPED (--skip-boot)"
    log "Step 4/4: SKIPPED (--skip-boot)"
    echo ""
    echo "====================================================="
    echo "POST-MERGE CHECK RESULT: PASS (build only)"
    echo "====================================================="
    exit 0
fi

log "Step 3/4: Boot xv6 under QEMU (timeout=${BOOT_TIMEOUT}s, CPUS=$CPUS)"

BOOT_LOG=$(mktemp /tmp/post_merge_boot_XXXXXX.log)

# Boot QEMU, wait for prompt, then send Ctrl-A X to quit.
# xv6 boots in < 2s; sleeping 4s gives sufficient margin while remaining fast.
(
    sleep 4
    printf '\x01x'
) | timeout "$BOOT_TIMEOUT" \
    qemu-system-riscv64 \
        -machine virt -bios none \
        -kernel kernel/kernel \
        -m 128M -smp "$CPUS" -nographic \
        -global virtio-mmio.force-legacy=false \
        -drive file=fs.img,if=none,format=raw,id=x0 \
        -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
    > "$BOOT_LOG" 2>&1 || true

# Check for successful boot indicators
if grep -q "init: starting sh" "$BOOT_LOG"; then
    pass "xv6 booted — 'init: starting sh' found"
    BOOT_OK=1
else
    fail "xv6 did not boot — 'init: starting sh' NOT found"
    echo "--- Boot log (last 30 lines) ---"
    tail -30 "$BOOT_LOG"
    echo "--- End boot log ---"
    BOOT_OK=0
fi

# ---- Step 4: Check for panics -----------------------------------
log "Step 4/4: Checking for panics in boot log"

if grep -qi "panic" "$BOOT_LOG"; then
    fail "Panic detected in boot log"
    grep -i "panic" "$BOOT_LOG"
    rm -f "$BOOT_LOG"
    exit 2
fi

rm -f "$BOOT_LOG"

if [ "$BOOT_OK" -eq 1 ]; then
    echo ""
    echo "====================================================="
    echo "POST-MERGE CHECK RESULT: PASS"
    echo "  Kernel built, fs.img built, xv6 booted at CPUS=$CPUS"
    echo "====================================================="
    exit 0
else
    echo ""
    echo "====================================================="
    echo "POST-MERGE CHECK RESULT: FAIL (boot)"
    echo "====================================================="
    exit 2
fi
