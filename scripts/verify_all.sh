#!/usr/bin/env bash
# Master Submission QA & Verification Script
set -euo pipefail
cd "$(dirname "$0")/.."

echo "========================================================================"
echo "      IIT JODHPUR — CSL3030 OPERATING SYSTEMS LAB — PROJECT 3 QA"
echo "========================================================================"
echo "Commit: $(git rev-parse HEAD)"
echo "Date:   $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
echo ""

echo "--- [1/5] Building Kernel and Disk Images ---"
make -j4 kernel/kernel fs.img
echo "BUILD PASS"
echo ""

echo "--- [2/5] Running Host Agent Lab Tests (39 Unit/Integration Tests) ---"
python3 -m unittest discover -s agent_lab/tests -v
echo ""

echo "--- [3/5] Running Guest Kernel Test Suites (QEMU CPUS=1) ---"
mkdir -p /tmp/xv6_qa_logs
for suite in ttest ftest ptest atest; do
  echo "Executing guest suite: $suite ..."
  python3 -m scripts.run_suite "$suite" --cpus 1 --log "/tmp/xv6_qa_logs/${suite}.log"
done
echo "ALL GUEST SUITES PASSED"
echo ""

echo "--- [4/5] Running Layer-1 Injection Defense Demo (INT1–INT8) ---"
bash agent_lab/make_lab_image.sh
python3 -m agent_lab.demo_agent
echo ""

echo "--- [5/5] Verifying Submission Packages & Checksums ---"
cd submissions
sha256sum -c MANIFEST.sha256
cd ..
bash scripts/verify_individual.sh
echo ""

echo "========================================================================"
echo "  VERIFICATION COMPLETE: ALL GATES (G1–G7) PASS ACCORDING TO SPEC"
echo "========================================================================"
