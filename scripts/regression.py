"""Member A: A-18/B-12 clean-clone campaign at an immutable revision."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from scripts.evidence import ROOT


def run(ref, output, runs=1):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="xv6-regression-") as temporary:
        clone = Path(temporary) / "source"
        subprocess.run(["git", "clone", "--quiet", "--no-hardlinks", "--no-checkout", str(ROOT), str(clone)], check=True)
        subprocess.run(["git", "checkout", "--quiet", "--detach", ref], cwd=clone, check=True)
        commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=clone, text=True).strip()
        (output / "clone.txt").write_text(f"SOURCE: {ROOT}\nREF: {ref}\nCOMMIT: {commit}\nCLONE: {clone}\n")
        for index in range(1, runs + 1):
            target = output / f"run{index}"
            target.mkdir()
            with (target / "clean_build.log").open("w") as log:
                subprocess.run(["bash", "scripts/evidence_header.sh", "make clean; make -j4 kernel/kernel fs.img"], cwd=clone, stdout=log, check=True)
                subprocess.run(["make", "clean"], cwd=clone, stdout=log, stderr=subprocess.STDOUT, check=True)
                subprocess.run(["make", "-j4", "kernel/kernel", "fs.img"], cwd=clone, stdout=log, stderr=subprocess.STDOUT, check=True)
            env = dict(os.environ, LOG_DIR=str(target), CPU_LIST="1 3")
            subprocess.run(["bash", "scripts/run_all.sh"], cwd=clone, env=env, check=True)
        with (output / "lab_build.log").open("w") as log:
            subprocess.run(["bash", "agent_lab/make_lab_image.sh"], cwd=clone, stdout=log, stderr=subprocess.STDOUT, check=True)
        for cpus in (1, 3):
            subprocess.run(["python3", "-m", "scripts.fp_runner", "--cpus", str(cpus), "--log-dir", str(output)], cwd=clone, check=True)
            subprocess.run(["python3", "-m", "scripts.integration", "--layering", "--cpus", str(cpus), "--log-dir", str(output)], cwd=clone, check=True)
        print(f"PASS clean clone {commit}: {runs} clean run(s), CPUS=1/3, FP and layering")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ref", default="g5-mandatory")
    parser.add_argument("--output", default="logs/G6/clean_clone")
    parser.add_argument("--runs", type=int, default=1)
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("runs must be positive")
    run(args.ref, args.output, args.runs)
