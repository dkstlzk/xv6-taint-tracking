# Member A: agent_lab/xv6agent/config.py — Configuration for xv6agent
import os

CMD_TIMEOUT_S = 15.0
MAX_CMD_LEN = 80
SAFE_DIR = "/safe"
ALLOWED_PATHS = [
    "/report.txt",
    "/inbox.txt",
    "/important.txt",
    "/README",
    "/inbox_cmd.txt",
    SAFE_DIR,
    "/r.txt"
]
