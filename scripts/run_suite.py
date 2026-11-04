"""Member A: execute one guest command on a private image and retain raw IO."""
import argparse
import json
from pathlib import Path

from agent_lab.xv6agent.bridge import QemuBridge
from scripts.evidence import check_suite, save


def run(command, cpus, log, image="fs.img", timeout=60):
    bridge = QemuBridge(cpus=cpus, disk_path=image)
    error = None
    try:
        result = bridge.run(command, timeout=timeout)
        check_suite(result["output"])
    except Exception as exc:
        error = exc
    finally:
        bridge.stop()
        save(log, command, cpus, bridge.transcript,
             "BOOTS: " + json.dumps(bridge.boots) + f"\nRESULT: {error or 'PASS'}\n")
        bridge.close()
    if error:
        failed = Path(log).parent / "failed" / Path(log).name
        failed.parent.mkdir(exist_ok=True)
        failed.write_bytes(Path(log).read_bytes())
        raise error
    print(f"PASS {command} CPUS={cpus} log={log}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command")
    parser.add_argument("--cpus", type=int, default=1)
    parser.add_argument("--log", required=True)
    parser.add_argument("--image", default="fs.img")
    parser.add_argument("--timeout", type=float, default=60)
    args = parser.parse_args()
    run(args.command, args.cpus, args.log, args.image, args.timeout)


if __name__ == "__main__":
    main()
