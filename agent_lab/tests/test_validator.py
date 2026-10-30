# agent_lab/tests/test_validator.py — 19+ Host unit tests for validate_action
import unittest
from agent_lab.xv6agent.tools import validate_action, ValidationError

class TestValidator(unittest.TestCase):
    def test_01_valid_list_files(self):
        validate_action({"tool": "list_files", "args": {}})

    def test_02_valid_read_file(self):
        validate_action({"tool": "read_file", "args": {"path": "/report.txt"}})

    def test_03_valid_read_file_safe(self):
        validate_action({"tool": "read_file", "args": {"path": "/safe/echo"}})

    def test_04_valid_summarize(self):
        validate_action({"tool": "summarize", "args": {"text": "hello world"}})

    def test_05_unknown_tool(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "delete_file", "args": {"path": "/important.txt"}})

    def test_06_unknown_tool_shell(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "execute_shell", "args": {"command": "rm important.txt"}})

    def test_07_action_not_dict(self):
        with self.assertRaises(ValidationError):
            validate_action(["list_files"])

    def test_08_action_missing_tool(self):
        with self.assertRaises(ValidationError):
            validate_action({"args": {}})

    def test_09_action_missing_args(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "list_files"})

    def test_10_extra_keys_in_action(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "list_files", "args": {}, "extra": 123})

    def test_11_list_files_with_args(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "list_files", "args": {"dir": "/"}})

    def test_12_read_file_missing_path(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {}})

    def test_13_read_file_relative_path(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {"path": "report.txt"}})

    def test_14_read_file_dot_dot(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {"path": "/safe/../important.txt"}})

    def test_15_read_file_long_component(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {"path": "/this_component_is_way_too_long_for_xv6.txt"}})

    def test_16_read_file_disallowed_path(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {"path": "/etc/passwd"}})

    def test_17_read_file_invalid_chars(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "read_file", "args": {"path": "/bad$name.txt"}})

    def test_18_summarize_missing_text(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "summarize", "args": {}})

    def test_19_summarize_wrong_type(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "summarize", "args": {"text": 12345}})

    def test_20_summarize_too_long(self):
        with self.assertRaises(ValidationError):
            validate_action({"tool": "summarize", "args": {"text": "A" * 4001}})

if __name__ == "__main__":
    unittest.main()
