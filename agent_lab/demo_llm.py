"""Member A: Live LLM Agent Demonstration.

Demonstrates real model-guided tool use or graceful fallback, passing through
Layer 1 validation and observing Layer 2 kernel enforcement.
"""
import os
import sys
from pathlib import Path
from agent_lab.xv6agent.bridge import QemuBridge
from agent_lab.xv6agent.agent import AgentRunner
from agent_lab.xv6agent.planner import ModelPlanner

def run_demo(request="Summarize the system financial report"):
    print("=" * 60)
    print("LIVE LLM AGENT DEMONSTRATION (Layer 1 + Layer 2)")
    print("=" * 60)
    provider = os.environ.get("LLM_PROVIDER")
    if not provider:
        if os.environ.get("GEMINI_API_KEY"):
            provider = "gemini (via GEMINI_API_KEY)"
        elif os.environ.get("OPENAI_API_KEY"):
            provider = "openai (via OPENAI_API_KEY)"
        elif os.environ.get("OLLAMA_HOST"):
            provider = "ollama (via OLLAMA_HOST)"
        else:
            provider = "deterministic fallback (no API keys in environment)"
    print(f"Active Provider: {provider}")
    print(f"User Request: {request}")
    print("-" * 60)

    # Initialize bridge and agent runner with ModelPlanner
    with QemuBridge(disk_path="lab.img" if Path("lab.img").exists() else "fs.img") as bridge:
        runner = AgentRunner(bridge=bridge, planner=ModelPlanner(fallback=True))
        for step in range(3):
            trace = runner.run_step(request, step)
            print(f"Step {step}:")
            print(f"  Proposed Action:     {trace['proposed_action']}")
            print(f"  Validator Decision:  {trace['validator_decision']}")
            print(f"  Command Dispatched:  {trace['command_sent']}")
            print(f"  Guest / Tool Output: {repr(trace['guest_output'])[:100]}")
            if trace['kernel_logs']:
                print(f"  Kernel Logs:         {trace['kernel_logs']}")
            if trace['validator_decision'] == "REJECTED":
                print("  => BLOCKED AT LAYER 1 (Tool-Layer Defense)!")
                break
            if not trace['proposed_action']:
                print("  => Goal achieved or planner terminated.")
                break
            print()
    print("=" * 60)
    print("Demo completed successfully.")

if __name__ == "__main__":
    req = sys.argv[1] if len(sys.argv) > 1 else "Summarize the system financial report"
    run_demo(req)
