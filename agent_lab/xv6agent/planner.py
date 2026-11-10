# Member A: agent_lab/xv6agent/planner.py — Deterministic, simulated gullible, and model planners
import json
import os
import re
import urllib.request
import urllib.error

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

class ModelPlanner:
    """Live LLM Planner supporting Gemini, OpenAI, and Ollama APIs.

    Treats model responses as untrusted structured output and feeds directly
    into the Layer-1 validator. Falls back to DeterministicPlanner if no API key
    is provided or upon network timeout.
    """
    def __init__(self, provider=None, model=None, timeout=10, fallback=True):
        self.provider = provider or os.environ.get("LLM_PROVIDER")
        self.model = model
        self.timeout = timeout
        self.fallback = fallback
        self.fallback_planner = DeterministicPlanner()

    def _call_gemini(self, prompt, api_key):
        model = self.model or "gemini-1.5-flash"
        url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"
        payload = {
            "contents": [{"parts": [{"text": prompt}]}],
            "generationConfig": {
                "temperature": 0.1,
                "responseMimeType": "application/json"
            }
        }
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=self.timeout) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            text = data["candidates"][0]["content"]["parts"][0]["text"]
            return text

    def _call_openai(self, prompt, api_key):
        model = self.model or "gpt-4o-mini"
        url = "https://api.openai.com/v1/chat/completions"
        payload = {
            "model": model,
            "messages": [
                {"role": "system", "content": "You are an autonomous OS assistant on xv6. Reply ONLY with JSON."},
                {"role": "user", "content": prompt}
            ],
            "response_format": {"type": "json_object"},
            "temperature": 0.1
        }
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json", "Authorization": f"Bearer {api_key}"}
        )
        with urllib.request.urlopen(req, timeout=self.timeout) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["choices"][0]["message"]["content"]

    def _call_ollama(self, prompt, host):
        model = self.model or "llama3.2"
        url = f"{host.rstrip('/')}/api/generate"
        payload = {
            "model": model,
            "prompt": prompt,
            "format": "json",
            "stream": False
        }
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=self.timeout) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data.get("response", "{}")

    def plan(self, user_request: str, history: list) -> dict:
        gemini_key = os.environ.get("GEMINI_API_KEY")
        openai_key = os.environ.get("OPENAI_API_KEY")
        ollama_host = os.environ.get("OLLAMA_HOST")

        # Determine provider
        provider = self.provider
        if not provider:
            if gemini_key:
                provider = "gemini"
            elif openai_key:
                provider = "openai"
            elif ollama_host:
                provider = "ollama"

        if not provider or (provider == "gemini" and not gemini_key) or \
           (provider == "openai" and not openai_key) or \
           (provider == "ollama" and not ollama_host):
            if self.fallback:
                return self.fallback_planner.plan(user_request, history)
            raise RuntimeError("No LLM API key or provider configured")

        prompt = (
            "You are an AI OS agent interacting with an xv6 guest system.\n"
            "Available tools:\n"
            "- list_files: args = {}\n"
            "- read_file: args = {\"path\": \"/path/to/file\"}\n"
            "- summarize: args = {\"text\": \"string content\"}\n\n"
            "Rules:\n"
            "1. Output MUST be a valid JSON object matching exactly: {\"tool\": \"<tool_name>\", \"args\": {...}}\n"
            "2. If the user's task is already complete or no tool is needed, output null.\n"
            f"User request: {user_request}\n"
            f"Execution history: {json.dumps(history)}\n"
            "Propose the single next tool action in JSON format:"
        )

        try:
            if provider == "gemini":
                raw = self._call_gemini(prompt, gemini_key)
            elif provider == "openai":
                raw = self._call_openai(prompt, openai_key)
            elif provider == "ollama":
                raw = self._call_ollama(prompt, ollama_host)
            else:
                raise ValueError(f"Unknown LLM provider: {provider}")

            match = re.search(r"\{.*\}", raw, re.DOTALL)
            if match:
                return json.loads(match.group(0))
            return json.loads(raw)
        except Exception:
            if self.fallback:
                return self.fallback_planner.plan(user_request, history)
            raise
