"""Member A: T2/T17/INT9 same-image reboot and fresh-image negative controls."""
import argparse
import json
from pathlib import Path
import secrets

from agent_lab.xv6agent.bridge import QemuBridge
from scripts.evidence import check_suite, save


def run_case(case, log_dir, cpus=1, fresh=False, abrupt=False):
    bridge = QemuBridge(cpus=cpus, persist=not fresh)
    token = secrets.token_hex(12)
    evidence = {"case": case, "token": token, "fresh_control": fresh, "abrupt": abrupt}
    error = None
    try:
        check_suite(bridge.run(f"ftest phase1-{case} {token}")["output"])
        bridge.stop(abrupt=abrupt)
        evidence["before_boot2"] = {"disk_path": bridge.disk_path,
                                    "sha256": bridge.get_disk_checksum()}
        bridge.start()
        evidence["boot2"] = bridge.boots[-1]
        proof = (evidence["before_boot2"] == evidence["boot2"] and
                 evidence["boot2"]["sha256"] != "NO_DISK")
        if case == "t17":
            listing = bridge.run("ls /")["output"]
            assert "t17a.txt" not in listing
        result = bridge.run(f"ftest phase2-{case} {token}")["output"]
        if fresh:
            assert "FAIL T2 " in result and "SUMMARY pass=0 fail=1 inconclusive=0" in result
            assert not proof
            evidence["result"] = "PASS negative control: fresh image fails T2"
        else:
            if not proof:
                evidence["result"] = "INCONCLUSIVE: missing/mismatched same-image evidence"
                raise ValueError(evidence["result"])
            check_suite(result)
            evidence["result"] = "PASS INT9/T2" if case == "t2" else "PASS T17"
    except Exception as exc:
        error = exc
        evidence.setdefault("result", "FAIL " + repr(exc))
    finally:
        suffix = "fresh_negative" if fresh else "killreboot" if abrupt else case
        save(Path(log_dir) / f"{suffix}_cpus{cpus}.log",
             f"ftest phase1/phase2-{case}; reboot (fresh={fresh}, abrupt={abrupt})",
             cpus, bridge.transcript, json.dumps(evidence, indent=2) + "\n")
        bridge.close()
    if error:
        raise error
    print(evidence["result"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--log-dir", default="logs/G5/day6")
    parser.add_argument("--cpus", type=int, default=1)
    parser.add_argument("--abrupt", action="store_true")
    args = parser.parse_args()
    if args.abrupt:
        run_case("t2", args.log_dir, args.cpus, abrupt=True)
    else:
        run_case("t2", args.log_dir, args.cpus)
        run_case("t17", args.log_dir, args.cpus)
        run_case("t2", args.log_dir, args.cpus, fresh=True)
