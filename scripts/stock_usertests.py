"""Member A (B-12): stock-UPROGS image, patched kernel, complete usertests."""
import argparse
from pathlib import Path
import subprocess

from agent_lab.xv6agent.bridge import QemuBridge
from scripts.evidence import header, save


def run(cpus, log_dir):
    subprocess.run(["make", "kernel/kernel", "stock.img"], check=True)
    command = "usertests (stock UPROGS; patched kernel)"
    provenance = header(command, cpus)
    bridge = QemuBridge(cpus=cpus, disk_path="stock.img")
    error = None
    try:
        result = bridge.run("usertests", timeout=1800)
        if "ALL TESTS PASSED" not in result["output"] or "FAILED" in result["output"]:
            raise AssertionError("stock usertests did not pass completely")
    except Exception as exc:
        error = exc
    finally:
        save(Path(log_dir) / f"usertests_cpus{cpus}.log", command, cpus,
             bridge.transcript, f"RESULT: {repr(error) if error else 'PASS'}\n", provenance)
        bridge.close()
    if error:
        raise error
    print(f"PASS full stock usertests CPUS={cpus}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpus", type=int, default=1)
    parser.add_argument("--log-dir", default="logs/G6")
    args = parser.parse_args()
    run(args.cpus, args.log_dir)
