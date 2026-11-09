# Member A: agent_lab/xv6agent/agent.py — Agent loop with frozen 8-field trace
import json
import os
from .tools import validate_action, ValidationError
from .planner import DeterministicPlanner, SimulatedGulliblePlanner
from .bridge import QemuBridge

class AgentRunner:
    def __init__(self, bridge=None, planner=None):
        self.bridge = bridge or QemuBridge()
        self.planner = planner or SimulatedGulliblePlanner()
        self.history = []
        self.traces = []

    def run_step(self, user_request: str, step_index: int):
        # 1. Planner proposes action
        proposed = self.planner.plan(user_request, self.history)
        
        # 2. Layer 1 Validation
        val_decision = "ACCEPTED"
        cmd_sent = None
        guest_output = None
        kernel_logs = []

        try:
            validate_action(proposed)
        except ValidationError as e:
            val_decision = "REJECTED"
            guest_output = f"Validator error: {e}"

        # 3. Execution via bridge if accepted
        if val_decision == "ACCEPTED":
            tool = proposed["tool"]
            args = proposed["args"]
            if tool == "list_files":
                cmd_sent = "ls"
            elif tool == "read_file":
                cmd_sent = f"cat {args['path']}"
            elif tool == "summarize":
                cmd_sent = None # Host-side tool only

            if cmd_sent:
                res = self.bridge.run(cmd_sent)
                guest_output = res.get("output", "")
                kernel_logs = res.get("kernel_logs", [])
            else:
                guest_output = f"Summary: {args.get('text', '')[:50]}"

        # 4. Form frozen 8-field trace
        trace_record = {
            "request": user_request,
            "step_index": step_index,
            "proposed_action": proposed,
            "validator_decision": val_decision,
            "command_sent": cmd_sent,
            "guest_output": guest_output,
            "kernel_logs": kernel_logs,
            "final_file_state": "not_checked"
        }
        self.traces.append(trace_record)
        self.history.append({"tool": proposed.get("tool") if isinstance(proposed, dict) else None, "output": guest_output})
        return trace_record
