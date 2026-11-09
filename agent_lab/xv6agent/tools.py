# Member A: agent_lab/xv6agent/tools.py — Tools and validation logic for xv6agent
import re
from . import config

class ValidationError(Exception):
    pass

TOOLS = {
    "list_files": {
        "required": set(),
        "optional": set()
    },
    "read_file": {
        "required": {"path"},
        "optional": set()
    },
    "summarize": {
        "required": {"text"},
        "optional": set()
    }
}

def validate_path(path: str) -> None:
    if not isinstance(path, str):
        raise ValidationError("path must be a string")
    if not path.startswith("/"):
        raise ValidationError("path must be absolute (start with /)")
    if ".." in path:
        raise ValidationError("path must not contain '..'")
    if "/./" in path or path.endswith("/."):
        raise ValidationError("path must not contain relative '.' components")
    
    parts = [p for p in path.split("/") if p]
    for part in parts:
        if len(part) > 14:
            raise ValidationError(f"path component exceeds 14 characters: {part!r}")
        if not re.match(r"^[A-Za-z0-9._-]+$", part):
            raise ValidationError(f"path component contains invalid characters: {part!r}")
    
    if path not in config.ALLOWED_PATHS and not path.startswith("/safe/"):
        raise ValidationError(f"path not in allowed exercise allowlist: {path!r}")

def validate_action(action: dict) -> None:
    if not isinstance(action, dict) or set(action.keys()) != {"tool", "args"}:
        raise ValidationError("action must be a dict with exactly {'tool', 'args'}")
    name = action["tool"]
    args = action["args"]
    if not isinstance(name, str):
        raise ValidationError("tool name must be a string")
    if name not in TOOLS:
        raise ValidationError(f"unknown tool: {name!r}")
    if not isinstance(args, dict):
        raise ValidationError("args must be a dictionary")
    
    req = TOOLS[name]["required"]
    opt = TOOLS[name]["optional"]
    provided = set(args.keys())
    
    missing = req - provided
    if missing:
        raise ValidationError(f"missing required argument(s): {missing}")
    
    extra = provided - (req | opt)
    if extra:
        raise ValidationError(f"unexpected argument(s): {extra}")
    
    if name == "read_file":
        validate_path(args["path"])
    elif name == "summarize":
        text = args["text"]
        if not isinstance(text, str):
            raise ValidationError("text argument must be a string")
        if len(text) > 4000:
            raise ValidationError("text argument exceeds maximum length of 4000 characters")
    elif name == "list_files":
        pass
