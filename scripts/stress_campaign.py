"""Member A runner for D-13: a separate QEMU session under optional host load."""
import argparse
import subprocess
import sys
from scripts.run_suite import run


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=8101)
    parser.add_argument("--trials", type=int, default=60)
    parser.add_argument("--load", action="store_true")
    parser.add_argument("--log", required=True)
    args = parser.parse_args()
    worker = None
    try:
        if args.load:
            worker = subprocess.Popen([sys.executable, "-c", "while True: sum(range(100000))"])
        run(f"ptest stress {args.trials} {args.seed}", 3, args.log, timeout=180)
        with open(args.log, "a") as log:
            log.write(f"\nHOST_CPU_LOAD_WORKERS: {int(args.load)}\n")
    finally:
        if worker:
            worker.terminate()
            worker.wait()
