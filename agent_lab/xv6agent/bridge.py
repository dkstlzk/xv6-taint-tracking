"""Member A: real QEMU transport. Completion means shell completion, not exit 0.

Each session uses a private image copied from disk_path. Persistent restart keeps
that image; fresh restart restores the template. No make/rebuild occurs on boot.
Only trusted test/demo callers may enable shell operators.
"""
import hashlib
import os
from pathlib import Path
import re
import secrets
import select
import shlex
import shutil
import subprocess
import tempfile
import time
from . import config


class BridgeTimeout(RuntimeError):
    pass


class BridgeDesync(RuntimeError):
    pass


class QemuBridge:
    def __init__(self, qemu_cmd=None, persist=False, disk_path="fs.img",
                 cpus=1, trusted_demo=False, root=None):
        self.root = Path(root or Path(__file__).resolve().parents[2])
        self.template = (self.root / disk_path).resolve()
        self.kernel = self.root / "kernel/kernel"
        self.qemu_cmd = qemu_cmd
        self.persist = persist
        self.cpus = int(cpus)
        self.trusted_demo = trusted_demo
        self._tmp = tempfile.TemporaryDirectory(prefix="xv6-bridge-")
        self.disk_path = str(Path(self._tmp.name) / "fs.img")
        self.proc = None
        self.desynchronized = False
        self.transcript = ""
        self._buffer = ""
        self.boots = []

    def _read_until(self, pattern, timeout):
        deadline = time.monotonic() + timeout
        while True:
            match = pattern.search(self._buffer)
            if match:
                result = self._buffer[:match.start()]
                self._buffer = self._buffer[match.end():]
                return result
            if self.proc.poll() is not None:
                self.desynchronized = True
                raise BridgeDesync(f"QEMU exited: {self.proc.returncode}")
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                self.desynchronized = True
                raise BridgeTimeout("QEMU command timed out; restart required")
            ready, _, _ = select.select([self.proc.stdout], [], [], remaining)
            if ready:
                data = os.read(self.proc.stdout.fileno(), 65536)
                text = data.decode("utf-8", errors="replace").replace("\r", "")
                self.transcript += text
                self._buffer += text
                if "panic:" in text:
                    self.desynchronized = True
                    raise BridgeDesync("kernel panic; see transcript")

    def start(self):
        if self.proc is not None:
            raise BridgeDesync("already started")
        if not self.persist or not Path(self.disk_path).exists():
            shutil.copyfile(self.template, self.disk_path)
        self.boots.append({"disk_path": self.disk_path,
                           "sha256": self.get_disk_checksum()})
        if self.qemu_cmd:
            cmd = shlex.split(self.qemu_cmd) if isinstance(self.qemu_cmd, str) else list(self.qemu_cmd)
            cmd = [s.replace("{disk}", self.disk_path) for s in cmd]
        else:
            cmd = ["qemu-system-riscv64", "-machine", "virt", "-bios", "none",
                   "-kernel", str(self.kernel), "-m", "128M", "-smp", str(self.cpus),
                   "-nographic", "-global", "virtio-mmio.force-legacy=false",
                   "-drive", f"file={self.disk_path},if=none,format=raw,id=x0",
                   "-device", "virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0"]
        self._buffer = ""
        self.desynchronized = False
        self.proc = subprocess.Popen(cmd, cwd=self.root, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     bufsize=0)
        try:
            self._read_until(re.compile(r"\$ "), 30)
        except Exception:
            self.stop()
            raise

    def run(self, cmd, timeout=None):
        charset = r"[A-Za-z0-9 ._/>|;:\-]+" if self.trusted_demo else r"[A-Za-z0-9 ._/\-]+"
        if not isinstance(cmd, str) or not cmd or len(cmd) > config.MAX_CMD_LEN:
            raise ValueError("empty command or MAX_CMD_LEN exceeded")
        if re.fullmatch(charset, cmd) is None:
            raise ValueError("command contains invalid characters")
        if self.desynchronized:
            raise BridgeDesync("restart required after timeout/error")
        if self.proc is None:
            self.start()
        # sh.c has a 100-byte command buffer, including its trailing NUL.
        # Keep framing to 18 bytes so an 80-byte command plus newline fits.
        sentinel = "@" + secrets.token_hex(4) + "@"
        # Drain the following prompt before sending again: on CPUS=3 a late
        # prompt can split the next command's echo and defeat echo exclusion.
        pattern = re.compile(r"(?<!echo )" + re.escape(sentinel) + r"\n\$ ")
        self.proc.stdin.write(f"{cmd}; echo {sentinel}\n".encode())
        output = self._read_until(pattern, config.CMD_TIMEOUT_S if timeout is None else timeout)
        lines = output.splitlines()
        lines = [line for line in lines if f"echo {sentinel}" not in line]
        output = "\n".join(lines).strip()
        return {"cmd": cmd, "output": output, "sentinel": sentinel,
                "kernel_logs": [line[line.index("taint:"):] for line in lines if "taint:" in line]}

    def get_disk_checksum(self):
        path = Path(self.disk_path)
        if not path.exists():
            return "NO_DISK"
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def restart(self):
        self.stop()
        self.start()

    def stop(self, abrupt=False):
        if self.proc is not None:
            if self.proc.poll() is None:
                if abrupt:
                    self.proc.kill()
                else:
                    self.proc.stdin.write(b"\x01x")
                try:
                    self.proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    self.proc.kill()
                    self.proc.wait()
            self.proc.stdin.close()
            self.proc.stdout.close()
            self.proc = None

    def close(self):
        self.stop()
        self._tmp.cleanup()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.close()
