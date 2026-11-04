#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
LOG_DIR="${LOG_DIR:-logs/G5}"
mkdir -p "$LOG_DIR"
{
  bash scripts/evidence_header.sh 'make kernel/kernel fs.img; host tests; guest suites'
  make -j4 kernel/kernel fs.img
} > "$LOG_DIR/build.log" 2>&1
{
  bash scripts/evidence_header.sh 'python3 -m unittest discover -s agent_lab/tests -v'
  python3 -m unittest discover -s agent_lab/tests -v
} > "$LOG_DIR/host_tests.log" 2>&1
for cpu_count in ${CPU_LIST:-1 3}; do
  for suite in ttest ftest ptest atest; do
    python3 -m scripts.run_suite "$suite" --cpus "$cpu_count" \
      --log "$LOG_DIR/${suite}_cpus${cpu_count}.log"
  done
done
