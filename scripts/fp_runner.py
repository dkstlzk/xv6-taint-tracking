"""Member A: five handbook false-positive controls with raw guest evidence."""
import argparse
from pathlib import Path

from agent_lab.xv6agent.bridge import QemuBridge
from scripts.evidence import check_suite, save


def run(log_dir, cpus=1):
    for number in range(1, 6):
        bridge = QemuBridge(disk_path="lab.img", cpus=cpus, trusted_demo=True)
        error = None
        try:
            bridge.run("mark /inbox.txt")
            if number == 1:
                result = bridge.run("cat /inbox.txt")
                assert "INJECTION:" in result["output"] and not result["kernel_logs"]
            elif number == 2:
                result = bridge.run("cat /inbox.txt | wc")
                assert any(char.isdigit() for char in result["output"]) and not result["kernel_logs"]
            elif number == 3:
                result = bridge.run("cat /inbox.txt > /copy.txt")
                assert len(result["kernel_logs"]) == 1 and "write refused" in result["kernel_logs"][0]
                assert bridge.run("cat /copy.txt")["output"].strip("$ \n") == ""
            elif number == 4:
                check_suite(bridge.run("ftest fp4")["output"])
                assert "allowed" in bridge.run("cat /new.txt")["output"]
            else:
                bridge.run("echo ls > /fp5ls")
                bridge.run("echo rm /important.txt > /fp5rm")
                result = bridge.run("cat /inbox.txt /fp5ls /fp5rm | sh")
                assert sum("exec refused" in line for line in result["kernel_logs"]) == 2
                assert "CRITICAL SYSTEM DATABASE" in bridge.run("cat /important.txt")["output"]
        except Exception as exc:
            error = exc
        finally:
            save(Path(log_dir) / f"fp_{number}_cpus{cpus}.log", f"FP-{number}", cpus,
                 bridge.transcript, f"RESULT: {'FAIL ' + repr(error) if error else 'PASS'}\n")
            bridge.close()
        if error:
            raise error
        print(f"PASS FP-{number} CPUS={cpus}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--log-dir", default="logs/G5")
    parser.add_argument("--cpus", type=int, default=1)
    args = parser.parse_args()
    run(args.log_dir, args.cpus)
