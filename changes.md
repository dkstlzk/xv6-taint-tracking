# Individual Contribution Record Template (changes.md)

One record per changed function or section, kept in each member's `changes.md` (or individual submission directory). Typo-level edits need no record.

## Template

```text
Member: ___  Date: ________
File: ____________  Function/section: ____________
Purpose of change: __________________________________
Implementation decisions (and alternatives rejected): ________
Invariants relied on / lock and transaction state: ________
Related test IDs: ______  Test result (PASS/FAIL/INCONCLUSIVE, log file): ______
Commit hash or change reference: ________
Known limitations / unresolved issues: __________________
```

---

## Initial Member B Records (Day 1)

### Record B-01-01
- **Member**: B (Anshika Jha)
- **Date**: 2026-10-26
- **File**: `kernel/proc.h`, `kernel/proc.c`
- **Function/section**: `struct proc`, `allocproc()`, `freeproc()`, `kfork()`
- **Purpose of change**: Add `int tainted;` to `struct proc` and enforce deterministic lifecycle initialization, zeroing on free/reuse, and inheritance across `fork()`.
- **Implementation decisions**: Initialized `p->tainted = 0` in `allocproc` and `freeproc`. In `kfork`, copied `np->tainted = p->tainted` under `acquire(&np->lock)` immediately before transitioning state to `RUNNABLE`.
- **Invariants relied on**: `p->lock` held during state transition; child not visible to scheduler as RUNNABLE until taint is set.
- **Related test IDs**: T1 (clean process clean), T4 (child inherits parent taint).
- **Commit hash**: B-02 task commit.
- **Known limitations**: Requires system calls (`marktaint`, `istainted`) and `fileread` latch to exercise end-to-end dynamically.
