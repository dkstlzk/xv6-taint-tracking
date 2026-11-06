"""Member A: measured Runs A/B/C; Run A requires a built stock source tree."""
import argparse
import hashlib
from pathlib import Path

from agent_lab.xv6agent.bridge import QemuBridge
from scripts.evidence import save


def run_scenarios(baseline_root, log_dir="logs/G5/day6", cpus=1):
    for label in ("A", "B", "C"):
        bridge = QemuBridge(root=baseline_root if label == "A" else None,
                            cpus=cpus, trusted_demo=True)
        error = None
        try:
            bridge.run("echo protected > /important.txt")
            bridge.run("echo rm /important.txt > /inbox_cmd.txt")
            if label == "C":
                bridge.run("mark /inbox_cmd.txt")
            result = bridge.run("cat /inbox_cmd.txt | sh")
            final = bridge.run("cat /important.txt")["output"]
            if label == "C":
                assert len(result["kernel_logs"]) == 1 and "exec refused" in result["kernel_logs"][0]
                assert "protected" in final
            else:
                assert not result["kernel_logs"] and "cannot open" in final
        except Exception as exc:
            error = exc
        finally:
            save(Path(log_dir) / f"demo_kernel_{label}_cpus{cpus}.log", f"kernel demo Run {label}", cpus,
                 bridge.transcript, f"KERNEL_TREE: {bridge.root}\n"
                 f"KERNEL_SHA256: {hashlib.sha256(bridge.kernel.read_bytes()).hexdigest()}\n"
                 f"BASELINE_REVISION: {'f780d7850765d74cbdcd1fff4b822d8829f7c9bd' if label == 'A' else 'current project'}\n"
                 f"RESULT: {repr(error) if error else 'PASS'}\n")
            bridge.close()
        if error:
            raise error
        print(f"PASS Run {label}: {'survived' if label == 'C' else 'deleted'}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline-root", required=True)
    parser.add_argument("--log-dir", default="logs/G5/day6")
    parser.add_argument("--cpus", type=int, default=1)
    args = parser.parse_args()
    run_scenarios(args.baseline_root, args.log_dir, args.cpus)
