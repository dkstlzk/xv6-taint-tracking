import unittest
from unittest.mock import patch
from agent_lab.xv6agent.planner import ModelPlanner
from agent_lab.xv6agent.tools import validate_action

class TestModelPlanner(unittest.TestCase):
    def test_fallback_when_no_keys(self):
        # Without any keys, ModelPlanner falls back to DeterministicPlanner
        with patch.dict("os.environ", {}, clear=True):
            planner = ModelPlanner(fallback=True)
            action = planner.plan("inspect files", [])
            self.assertEqual(action, {"tool": "list_files", "args": {}})
            # Action must pass validate_action
            validate_action(action)

    def test_mock_gemini_response(self):
        mock_resp = '{"tool": "read_file", "args": {"path": "/report.txt"}}'
        with patch.dict("os.environ", {"GEMINI_API_KEY": "test_key"}):
            planner = ModelPlanner()
            with patch.object(planner, "_call_gemini", return_value=mock_resp):
                action = planner.plan("read report", [])
                self.assertEqual(action, {"tool": "read_file", "args": {"path": "/report.txt"}})
                validate_action(action)

    def test_mock_openai_response(self):
        mock_resp = '{"tool": "list_files", "args": {}}'
        with patch.dict("os.environ", {"OPENAI_API_KEY": "test_key"}):
            planner = ModelPlanner()
            with patch.object(planner, "_call_openai", return_value=mock_resp):
                action = planner.plan("what files exist", [])
                self.assertEqual(action, {"tool": "list_files", "args": {}})
                validate_action(action)

    def test_invalid_json_fallback(self):
        with patch.dict("os.environ", {"GEMINI_API_KEY": "test_key"}):
            planner = ModelPlanner(fallback=True)
            with patch.object(planner, "_call_gemini", return_value="Not valid JSON"):
                action = planner.plan("list files", [])
                self.assertEqual(action, {"tool": "list_files", "args": {}})

if __name__ == "__main__":
    unittest.main()
