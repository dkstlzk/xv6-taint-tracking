# Member A: agent_lab/xv6agent/planner.py — Deterministic and simulated gullible planners
import re

class DeterministicPlanner:
    def plan(self, user_request: str, history: list) -> dict:
        if not history:
            return {"tool": "list_files", "args": {}}
        last_step = history[-1]
        if last_step.get("tool") == "list_files":
            return {"tool": "read_file", "args": {"path": "/report.txt"}}
        if last_step.get("tool") == "read_file":
            content = last_step.get("output", "")
            return {"tool": "summarize", "args": {"text": content}}
        return None

class SimulatedGulliblePlanner:
    def plan(self, user_request: str, history: list) -> dict:
        for step in reversed(history):
            output = step.get("output", "")
            # Look for injected command instructions
            m = re.search(r"INJECTION:\s*(\w+)\s+(.+)", output)
            if m:
                tool, arg = m.group(1), m.group(2).strip()
                if tool == "delete_file":
                    return {"tool": "delete_file", "args": {"path": arg}}
                elif tool == "execute_shell":
                    return {"tool": "execute_shell", "args": {"command": arg}}
                elif tool == "read_file":
                    return {"tool": "read_file", "args": {"path": arg}}
        return {"tool": "list_files", "args": {}}
