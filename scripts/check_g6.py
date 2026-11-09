"""Member A/B: Gate G6 regression and validation check (Task X-10)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from scripts.evidence import check_suite, ROOT


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log-dir", default="logs/G6")
    args = parser.parse_args()
    directory = Path(args.log_dir)
    evidence = []

    def check_file(path):
        assert path.exists(), f"Missing evidence: {path}"
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        evidence.append({"path": str(path), "sha256": sha})
        return path.read_text()

    # 1. Three consecutive clean runs in final_clone at CPUS=1 and 3
    final_clone = directory / "final_clone"
    for run_idx in (1, 2, 3):
        run_dir = final_clone / f"run{run_idx}"
        for cpus in (1, 3):
            for suite in ("ttest", "ftest", "ptest", "atest"):
                text = check_file(run_dir / f"{suite}_cpus{cpus}.log")
                rows = check_suite(text.split("RAW TRANSCRIPT\n", 1)[1])
                assert len(rows) > 0, f"Empty suite {suite} in run{run_idx}"
            host_text = check_file(run_dir / "host_tests.log")
            assert "OK" in host_text, f"Host tests failed in run{run_idx}"

    # 2. Stock usertests on patched kernel
    for cpus in (1, 3):
        ut_text = check_file(directory / f"usertests_cpus{cpus}.log")
        assert "ALL TESTS PASSED" in ut_text, f"Stock usertests failed CPUS={cpus}"

    # 3. Stress campaigns (idle, load, final clean)
    for stress_name in ("stress_idle_cpus3.log", "stress_load_cpus3.log", "stress_final_cpus3.log"):
        stress_text = check_file(directory / stress_name)
        assert len(re.findall(r"false_negatives=0 order=", stress_text)) >= 60

    # 4. FP controls
    for cpus in (1, 3):
        for num in range(1, 6):
            fp_text = check_file(final_clone / f"fp_{num}_cpus{cpus}.log")
            assert "RESULT: PASS" in fp_text

    # 5. FP classification table (Task C-16)
    fp_class = check_file(directory / "fp_classification.md")
    assert "Allowed benign operation" in fp_class
    assert "Intended policy consequence" in fp_class
    assert "Zero implementation bugs identified" in fp_class

    # 6. Persistence repeats across three images (Task C-16)
    repeats_dir = directory / "persistence_repeats"
    for img_idx in (1, 2, 3):
        img_dir = repeats_dir / f"image{img_idx}"
        t2_text = check_file(img_dir / "t2_cpus1.log")
        assert "PASS INT9/T2" in t2_text
        t17_text = check_file(img_dir / "t17_cpus1.log")
        assert "PASS T17" in t17_text
        neg_text = check_file(img_dir / "fresh_negative_cpus1.log")
        assert "PASS negative control" in neg_text
        ft_text = check_file(img_dir / "ftest_cpus1.log")
        assert "PASS T13 REUSE-CONFIRMED" in ft_text

    # 7. Audit pass #3 (Task X-09)
    audit_text = check_file(ROOT / "logs" / "audit.md")
    assert "Day 9 pass #3 — X-09 Final Denial-Path Audit" in audit_text
    assert "All seven denial obligations checked" in audit_text

    # 8. Layering and integration
    for cpus in (1, 3):
        integ_text = check_file(final_clone / f"integration_cpus{cpus}.log")
        for num in (1, 2, 3, 4, 5, 6, 7, 8, 10, 11):
            assert f"PASS INT{num} " in integ_text

    report = {
        "gate": "G6",
        "result": "PASS",
        "status": "All clean-clone runs, stock usertests, stress campaigns, FP classification, persistence repeats, and final audit pass #3 validated",
        "evidence_count": len(evidence),
        "evidence": evidence
    }
    (directory / "gate_g6.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS G6: regression and security validation confirmed across {len(evidence)} evidence files.")


if __name__ == "__main__":
    main()
