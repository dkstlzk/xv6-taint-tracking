# Plan-Deviation Records (PLAN_DEVIATIONS.md)

Required by Lab 9 Principle P4. Any departure from the handbook, handout design, or frozen contracts is recorded before it is merged, after unanimous agreement.

## Template

```text
ID: DEV-__  
Date: ______  
Proposed by: ___  
What changed (file, interface, test, schedule): ________  
Why it was necessary (evidence: log, failing test, source finding): ________  
Components and members affected: ________  
Impact on tests, interfaces, other deviations: ________  
Agreed by: A __ B __ C __ D __  
Explained by (member who can defend it): ___  
```

---

## Logged Deviations

### DEV-01: Integration of Stock xv6 Source Tree into Root Repository Workspace
- **Date**: 2026-10-26
- **Proposed by**: Member B
- **What changed**: Populated root repository with the stock xv6-riscv source tree matching pinned commit `f780d7850765d74cbdcd1fff4b822d8829f7c9bd`, co-located with `scripts/`, `logs/`, and project documentation.
- **Why it was necessary**: Initial baseline exploration was executed against an untracked `external/xv6-riscv` directory ignored by `.gitignore`. Placing xv6 directly in the repository root allows git to track and attribute all subsequent kernel and user edits (`kernel/proc.*`, `user/atest.c`, `user/ttest.c`, etc.) in compliance with Lab 9 principles PR1 and PR2.
- **Components and members affected**: All members; `Makefile`, `scripts/post_merge_check.sh`, `scripts/evidence_header.sh`.
- **Impact on tests, interfaces, other deviations**: `scripts/post_merge_check.sh` checks `$PROJECT_ROOT/kernel` directly before falling back to `$XV6_DIR`.
- **Agreed by**: A [x] B [x] C [x] D [x]
- **Explained by**: Member B
