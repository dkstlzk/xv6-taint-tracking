"""Member A: provenance, raw transcripts, and strict suite result parsing."""
import hashlib
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
RESULT = re.compile(r"^(PASS|FAIL|INCONCLUSIVE)\s+(\S+)\s+(.+)$", re.M)
SUMMARY = re.compile(r"^SUMMARY pass=(\d+) fail=(\d+) inconclusive=(\d+)$", re.M)


def header(command, cpus):
    env = dict(os.environ, CPUS=str(cpus))
    result = subprocess.check_output(["bash", "scripts/evidence_header.sh", command],
                                     cwd=ROOT, env=env, text=True)
    diff = subprocess.check_output(["git", "diff", "HEAD", "--", "kernel", "user",
                                    "agent_lab", "scripts", "Makefile"], cwd=ROOT)
    return result + f"SOURCE_DIFF_SHA256: {hashlib.sha256(diff).hexdigest()}\nSOURCE_DIRTY: {bool(diff)}\n"


def save(path, command, cpus, transcript, details="", provenance=None):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text((provenance if provenance is not None else header(command, cpus)) + details + "\nRAW TRANSCRIPT\n" + transcript)


def check_suite(output):
    lines = re.sub(r"(?m)^(?:\$ )+", "", output)
    summaries = SUMMARY.findall(lines)
    if len(summaries) != 1:
        raise ValueError("missing or multiple SUMMARY lines: incomplete suite")
    counts = tuple(map(int, summaries[0]))
    results = RESULT.findall(lines)
    actual = tuple(sum(row[0] == kind for row in results)
                   for kind in ("PASS", "FAIL", "INCONCLUSIVE"))
    if counts != actual or counts[0] == 0 or counts[1:] != (0, 0):
        raise ValueError(f"suite failed: SUMMARY={counts}, observed={actual}")
    return results
