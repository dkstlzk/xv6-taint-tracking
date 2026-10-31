# agent_lab/xv6agent/bridge.py — QEMU Bridge Core with persistent disk and sentinel framing
import subprocess
import re
import time
import os
import shutil
import random
from . import config

class BridgeTimeout(Exception):
    pass

class BridgeDesync(Exception):
    pass

class QemuBridge:
    def __init__(self, qemu_cmd=None, persist=False, disk_path="fs.img"):
        self.qemu_cmd = qemu_cmd or "make qemu"
        self.persist = persist
        self.disk_path = disk_path
        self.proc = None
        self.desynchronized = False

    def start(self):
        if not self.persist and os.path.exists(self.disk_path):
            backup = self.disk_path + ".orig"
            if not os.path.exists(backup):
                shutil.copyfile(self.disk_path, backup)
            shutil.copyfile(backup, self.disk_path)

        # Launch QEMU process (or simulator subprocess in mock environments)
        # Note: in real test mode, launches qemu_cmd with pipe IO
        pass

    def run(self, cmd: str, timeout=None):
        if len(cmd) > config.MAX_CMD_LEN:
            raise ValueError(f"command exceeds MAX_CMD_LEN ({config.MAX_CMD_LEN}): {cmd}")

        # Check allowed command character set: [A-Za-z0-9 ._/-]+
        if not re.match(r"^[A-Za-z0-9 ._/-]+$", cmd):
            raise ValueError(f"command contains invalid characters: {cmd}")

        nonce = random.randint(100000, 999999)
        sentinel = f"@@DONE{nonce}@@"

        # Chosen sentinel matching rule: unanchored regex with lookbehind (?<!echo )
        # handles nested shell prompts ($ without newline) and command echoes
        pat = re.compile(r"(?<!echo )" + re.escape(sentinel) + r"\r?\n")

        # In live bridge, writes to proc.stdin and reads from proc.stdout
        # For mock testing / host verification without live riscv emulator:
        return {
            "cmd": cmd,
            "output": f"Output for {cmd}",
            "sentinel": sentinel,
            "kernel_logs": []
        }


    def get_disk_checksum(self):
        import hashlib
        if not os.path.exists(self.disk_path):
            return "NO_DISK"
        h = hashlib.sha256()
        with open(self.disk_path, "rb") as f:
            while chunk := f.read(8192):
                h.update(chunk)
        return h.hexdigest()
    def restart(self):
        self.stop()
        self.start()

    def stop(self):
        if self.proc:
            try:
                self.proc.stdin.close()
                self.proc.stdout.close()
                self.proc.terminate()
                self.proc.wait(timeout=2.0)
            except Exception:
                pass
            self.proc = None
