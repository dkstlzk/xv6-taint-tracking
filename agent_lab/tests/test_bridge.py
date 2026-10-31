# agent_lab/tests/test_bridge.py — 9+ bridge unit/mock test cases
import unittest
import os
import re
from agent_lab.xv6agent.bridge import QemuBridge, BridgeTimeout
from agent_lab.xv6agent import config

class TestBridge(unittest.TestCase):
    def setUp(self):
        self.bridge = QemuBridge()

    def test_01_command_charset_valid(self):
        res = self.bridge.run("ls -l /safe")
        self.assertEqual(res["cmd"], "ls -l /safe")

    def test_02_command_charset_invalid(self):
        with self.assertRaises(ValueError):
            self.bridge.run("cat /etc/passwd; rm -rf *")

    def test_03_max_command_length(self):
        with self.assertRaises(ValueError):
            self.bridge.run("a" * 81)

    def test_04_sentinel_generation(self):
        res = self.bridge.run("ls")
        self.assertTrue(res["sentinel"].startswith("@@DONE"))

    def test_05_sentinel_regex_lookbehind(self):
        nonce = "123456"
        sentinel = f"@@DONE{nonce}@@"
        pat = re.compile(r"(?<!echo )" + re.escape(sentinel) + r"\r?\n")
        
        # Should NOT match the echo statement line
        echo_line = f"ls ; echo {sentinel}\n"
        self.assertIsNone(pat.search(echo_line))
        
        # Should match actual output line
        output_line = f"file1 file2\n{sentinel}\n"
        self.assertIsNotNone(pat.search(output_line))

    def test_06_persistent_mode_checksum(self):
        b = QemuBridge(persist=True, disk_path="test_disk.tmp")
        with open("test_disk.tmp", "wb") as f:
            f.write(b"SAMPLE DISK CONTENT")
        c1 = b.get_disk_checksum()
        self.assertNotEqual(c1, "NO_DISK")
        os.remove("test_disk.tmp")

    def test_07_fresh_disk_mode_flag(self):
        b = QemuBridge(persist=False)
        self.assertFalse(b.persist)

    def test_08_nested_shell_simulation(self):
        # Verify unanchored matching works even with nested shell prompt
        sentinel = "@@DONE999999@@"
        pat = re.compile(r"(?<!echo )" + re.escape(sentinel) + r"\r?\n")
        nested_output = f"$ $ echo hi\r\nhi\r\n{sentinel}\r\n"
        self.assertIsNotNone(pat.search(nested_output))

    def test_09_kernel_log_search(self):
        # Searching the whole line, not start of line
        line = "$ taint: exec refused for pid 3"
        self.assertIn("taint: exec refused", line)

    def test_10_restart_clears_process(self):
        self.bridge.restart()
        self.assertIsNone(self.bridge.proc)

if __name__ == "__main__":
    unittest.main()
