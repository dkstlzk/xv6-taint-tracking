#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

if [ $# -eq 0 ]; then
  echo "Verifying all individual packages and team package..."
  for pkg in submissions/member_A.zip submissions/member_B.zip submissions/member_C.zip submissions/member_D.zip submissions/team.zip; do
    if [ -f "$pkg" ]; then
      python3 -m scripts.packaging verify "$pkg"
    fi
  done
  echo "ALL SUBMISSION PACKAGES VERIFIED SUCCESSFULLY!"
else
  python3 -m scripts.packaging verify "$@"
fi
