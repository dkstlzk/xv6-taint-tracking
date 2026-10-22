#!/usr/bin/env bash
# evidence_header.sh — Emit the evidence header required by the project.
#
# Every transcript / log file starts with this header so that evidence
# can be tied to a specific commit, environment and command.
#
# Usage:
#   source scripts/evidence_header.sh          # prints to stdout
#   scripts/evidence_header.sh                 # same
#   scripts/evidence_header.sh | tee -a LOG    # capture into a log
#   scripts/evidence_header.sh  "<command>"    # also records the command
#
# Output (one field per line):
#   COMMIT:       <full SHA of HEAD>
#   DATE:         <ISO-8601 date/time with timezone>
#   CPUS:         <value of $CPUS or "not set (default 3)">
#   GCC_VERSION:  <first line of cross-compiler --version>
#   QEMU_VERSION: <first line of QEMU --version>
#   COMMAND:      <the command that will produce the evidence>
#
# Requirement source:
#   Handbook §503–504 (Evidence convention): "git rev-parse HEAD, date,
#   CPUS, riscv64-*-gcc --version, qemu-system-riscv64 --version, and
#   the exact command line."
#   Plan: A owns scripts/. The header script is not named by the plan;
#   A is the natural owner (handbook §504, PROPT).
#
# Exit codes:
#   0  — header printed successfully
#   1  — not inside a git repository (COMMIT will show the error)
#
# Author: Member A
# ---------------------------------------------------------------

set -euo pipefail

# --- Locate the project root (the git repository) ----------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# --- Locate the xv6 source tree -----------------------------------
if [ -d "$PROJECT_ROOT/kernel" ]; then
    XV6_DIR="${XV6_DIR:-$PROJECT_ROOT}"
else
    XV6_DIR="${XV6_DIR:-$PROJECT_ROOT/external/xv6-riscv}"
fi

# --- Detect the cross-compiler prefix ----------------------------
detect_toolprefix() {
    local prefixes=(
        riscv64-unknown-elf-
        riscv64-elf-
        riscv64-none-elf-
        riscv64-linux-gnu-
        riscv64-unknown-linux-gnu-
    )
    for p in "${prefixes[@]}"; do
        if command -v "${p}gcc" &>/dev/null; then
            echo "$p"
            return
        fi
    done
    echo ""
}

TOOLPREFIX="$(detect_toolprefix)"

# --- Gather values ------------------------------------------------
COMMIT="$(cd "$PROJECT_ROOT" && git rev-parse HEAD 2>/dev/null || echo 'NOT A GIT REPO')"
DATE="$(date --iso-8601=seconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')"

if [ -n "${CPUS:-}" ]; then
    CPUS_VAL="$CPUS"
else
    CPUS_VAL="not set (default 3)"
fi

if [ -n "$TOOLPREFIX" ] && command -v "${TOOLPREFIX}gcc" &>/dev/null; then
    GCC_VERSION="$("${TOOLPREFIX}gcc" --version 2>&1 | head -1)"
else
    GCC_VERSION="NOT FOUND (riscv64 compiler missing)"
fi

if command -v qemu-system-riscv64 &>/dev/null; then
    QEMU_VERSION="$(qemu-system-riscv64 --version 2>&1 | head -1)"
else
    QEMU_VERSION="NOT FOUND (qemu-system-riscv64 missing)"
fi

COMMAND_LINE="${1:-<not specified>}"

# --- Print --------------------------------------------------------
cat <<EOF
================================================================
EVIDENCE HEADER
================================================================
COMMIT:       $COMMIT
DATE:         $DATE
CPUS:         $CPUS_VAL
GCC_VERSION:  $GCC_VERSION
QEMU_VERSION: $QEMU_VERSION
COMMAND:      $COMMAND_LINE
================================================================
EOF

# Return non-zero only if we could not determine the commit.
[[ "$COMMIT" != "NOT A GIT REPO" ]]
