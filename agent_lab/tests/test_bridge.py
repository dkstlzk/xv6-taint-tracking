import unittest
from agent_lab.xv6agent.bridge import QemuBridge, BridgeDesync, BridgeTimeout


class TestBridge(unittest.TestCase):
    def setUp(self):
        self.bridge = QemuBridge(trusted_demo=True)
        self.addCleanup(self.bridge.close)

    def test_listing_is_real(self):
        self.assertIn("README", self.bridge.run("ls")["output"])

    def test_default_rejects_shell_operators(self):
        self.bridge.trusted_demo = False
        with self.assertRaises(ValueError):
            self.bridge.run("ls; rm README")
        self.assertIsNone(self.bridge.proc)

    def test_max_length(self):
        with self.assertRaises(ValueError):
            self.bridge.run("a" * 81)

    def test_eighty_byte_command_fits_guest_shell(self):
        payload = "x" * 75
        self.assertIn(payload, self.bridge.run("echo " + payload)["output"])

    def test_empty_and_newline(self):
        for command in ("", "ls\necho bad"):
            with self.assertRaises(ValueError):
                self.bridge.run(command)

    def test_sentinel_does_not_match_echo(self):
        result = self.bridge.run("echo actual-output")
        self.assertIn("actual-output", result["output"])
        self.assertNotIn(result["sentinel"], result["output"])
        self.assertIn(result["sentinel"], self.bridge.transcript)

    def test_nested_shell(self):
        self.assertIn("nested", self.bridge.run("echo echo nested | sh")["output"])
        self.assertIn("next", self.bridge.run("echo next")["output"])

    def test_file_without_trailing_newline(self):
        self.bridge.run("ftest fp4")
        self.assertIn("allowed", self.bridge.run("cat /new.txt")["output"])

    def test_three_cpu_commands_wait_for_prompt(self):
        self.bridge.cpus = 3
        for index in range(50):
            payload = f"frame-{index}"
            result = self.bridge.run("echo " + payload)
            self.assertEqual(result["output"], payload)

    def test_timeout_requires_restart(self):
        with self.assertRaises(BridgeTimeout):
            self.bridge.run("cat", timeout=0.3)
        with self.assertRaises(BridgeDesync):
            self.bridge.run("ls")
        self.bridge.restart()
        self.assertIn("README", self.bridge.run("ls")["output"])

    def test_persistent_restart(self):
        self.bridge.persist = True
        self.bridge.run("echo retained > /bridge.txt")
        self.bridge.run("sync")
        self.bridge.stop()
        checksum = self.bridge.get_disk_checksum()
        self.bridge.start()
        self.assertEqual(checksum, self.bridge.boots[-1]["sha256"])
        self.assertIn("retained", self.bridge.run("cat /bridge.txt")["output"])

    def test_fresh_restart(self):
        self.bridge.run("echo ephemeral > /bridge.txt")
        self.bridge.restart()
        self.assertIn("cannot open", self.bridge.run("cat /bridge.txt")["output"])

    def test_stop_reaps_qemu(self):
        self.bridge.start()
        process = self.bridge.proc
        self.bridge.stop()
        self.assertIsNotNone(process.poll())
        self.assertIsNone(self.bridge.proc)
