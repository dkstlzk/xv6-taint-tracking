"""G5 evidence checks, maintained by A for gate lead D (X-08)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from scripts.evidence import check_suite, ROOT


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log-dir", default="logs/G5/day7")
    args = parser.parse_args()
    directory = Path(args.log_dir)
    evidence = []
    tested = set()

    def read(name):
        path = directory / name
        text = path.read_text()
        commit = re.search(r"^COMMIT:\s+(\w+)$", text, re.M).group(1)
        subprocess.run(["git", "cat-file", "-e", commit + "^{commit}"], check=True, cwd=ROOT)
        assert "SOURCE_DIRTY: True" not in text, name
        tested.add(commit)
        evidence.append({"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "commit": commit})
        return text

    expected_logs = {"ttest": {"write": 1}, "ftest": {"write": 5, "unlink": 1},
                     "ptest": {"exec": 3}, "atest": {"unlink": 4, "write": 4, "link": 1, "exec": 1}}
    for cpus in (1, 3):
        for suite in ("ttest", "ftest", "ptest", "atest"):
            text = read(f"{suite}_cpus{cpus}.log")
            rows = check_suite(text.split("RAW TRANSCRIPT\n", 1)[1])
            for kind in ("write", "unlink", "link", "exec"):
                assert len(re.findall(r"taint: " + kind + r" refused for pid \d+", text)) == expected_logs[suite].get(kind, 0), (suite, kind)
            if suite == "atest":
                assert {row[1] for row in rows} >= {"A1", "A2", "A3", "A8", "A9", "A17", "A18"}
            if suite == "ftest":
                assert "REUSE-CONFIRMED" in text
        for case in ("t2", "t17"):
            text = read(f"{case}_cpus{cpus}.log")
            metadata = json.loads(text.split("SOURCE_DIRTY: False\n", 1)[1].split("\nRAW TRANSCRIPT", 1)[0])
            assert metadata["before_boot2"] == metadata["boot2"]
            assert metadata["result"].startswith("PASS")
        assert "PASS negative control" in read(f"fresh_negative_cpus{cpus}.log")
    stress = read("stress60_cpus3.log")
    assert len(check_suite(stress.split("RAW TRANSCRIPT\n", 1)[1])) >= 60
    assert len(re.findall(r"false_negatives=0 order=", stress)) >= 60
    assert re.search(r"Ran \d+ tests", read("host_tests.log"))
    assert "\nOK\n" in (directory / "host_tests.log").read_text()
    for number in range(1, 6):
        assert "RESULT: PASS" in read(f"fp_{number}_cpus1.log")
    integration = read("integration_cpus1.log")
    for number in range(1, 9):
        assert f"PASS INT{number} " in integration
    for label in ("A", "B", "C"):
        assert "RESULT: PASS" in read(f"demo_kernel_{label}_cpus1.log")
    assert len(tested) == 1, "gate campaign must name one tested commit"
    assert "Day 6 pass #2" in Path("logs/audit.md").read_text()
    assert "Retain the watermark design" in Path("logs/pipe_invariant.md").read_text()
    report = {"gate": "G5", "result": "PASS", "tested_commit": tested.pop(),
              "human_peer_signatures": "not asserted", "evidence": evidence}
    (directory / "gate_g5.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS G5: mandatory evidence validated; no open implementation P0")


if __name__ == "__main__":
    main()
