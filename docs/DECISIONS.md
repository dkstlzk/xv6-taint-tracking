# Decision Summary (DEC-1 to DEC-9)

| ID | Title | Selected Contract | Owner | Status |
|---|---|---|---|---|
| DEC-1 | Home of `in_safe()` / `ip_untrusted()` | New `kernel/taint.c` with declarations in `kernel/defs.h` | Member D | PROPOSED |
| DEC-2 | `exec.c` visibility of `ip->type` | Direct `#include` of `stat.h`, `sleeplock.h`, `fs.h`, `file.h` | Member D | CONFIRMED |
| DEC-3 | Centralization of `SAFE_DIR` & Log Strings | Centralized in `kernel/taint.h` with four strict refusal formats | Member D | PROPOSED |
| DEC-4 | Log string for `sys_open` write refusal | `taint: write refused for pid %d\n` (matches `filewrite`) | Member C | PROPOSED |
| DEC-5 | Provisioning of `/safe` directory | Created in guest by clean process prelude (`mkdir`/`link`) | Member D / A | PROPOSED (Provisional) |
| DEC-6 | Optional Hardening (OH1, OH2, OH3) | Retain OH1 (link) and OH3 (exec /safe); OH2 logs write refusal | Member C / D | PROPOSED |
| DEC-7 | Ownership of `mkfs/mkfs.c` | Member C; no edits needed under 64-byte dinode Option C | Member C | CONFIRMED |
| DEC-8 | Repository Paths & Automation Conventions | Root-level xv6 tree, standardized script names & log paths | Member B / A | PROPOSED |
| DEC-9 | Instructor Questions (U4–U10) | Consolidated Day-1 inquiry submitted jointly by Members A & B | Member A / B | CONFIRMED |

See full rationale, invariants, and alternatives in [CHANGES.md](../CHANGES.md).
