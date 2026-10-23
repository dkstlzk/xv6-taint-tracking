# Architectural Decisions & Frozen Contracts (DEC-1 through DEC-9)

In accordance with the CSL3030 Project 3 Four-Member Execution Plan (§1.4, §6.3, Table 1.1) and Final QA Specification (Appendix K), this document freezes the architectural contracts for the xv6 taint-tracking system.

Status note: In compliance with the verification rules, where a decision reflects the frozen contract from the official execution plan, it is recorded here as the authoritative team proposal awaiting final unanimous in-person ratification by all four members (A, B, C, D).

---

## DEC-1: Home of `in_safe()` and `ip_untrusted()`
- **Problem**: Where should the containment helper functions `in_safe(struct inode *ip)` and `ip_untrusted(struct inode *ip)` reside?
- **Decision**: Implemented in a dedicated kernel file `kernel/taint.c` owned by Member D, with function prototypes declared in `kernel/defs.h` within a clearly delimited block (`// taint.c (member D)`).
- **Rationale**: Keeps the entire `/safe` containment logic unified under Member D rather than overloading `fs.c` (Member C), decoupling inode management from path/containment policy.
- **Alternatives Considered**: Placing them in `kernel/fs.c` (rejected: would overload Member C and complicate attribution).
- **Affected Tasks & Modules**: D-02, D-05, C-02, `kernel/defs.h`, `kernel/taint.c`.
- **Verification**: Header check in `defs.h`, symbol link test in G1 (`taint.o` links with fail-closed stubs).
- **Approval Status**: PROPOSED (Plan default; frozen contract).

---

## DEC-2: `exec.c` Visibility of `ip->type`
- **Problem**: `kernel/exec.c` needs to inspect `ip->type` and `ip->untrusted` when verifying execution policy, but does not include `file.h` or `fs.h` by default.
- **Decision**: Add `#include "kernel/stat.h"`, `#include "kernel/sleeplock.h"`, `#include "kernel/fs.h"`, and `#include "kernel/file.h"` directly to `kernel/exec.c` (confirmed by Handbook §14.3 and Table 25.1 item X5).
- **Rationale**: Direct header inclusion is cleaner than adding extra wrapper functions and is idiomatic in xv6.
- **Alternatives Considered**: Adding helper query functions in `fs.c` (rejected: unnecessary indirection).
- **Affected Tasks & Modules**: D-02, D-05, `kernel/exec.c`.
- **Verification**: Compilation check in D-05.
- **Approval Status**: CONFIRMED (Verified against pinned source).

---

## DEC-3: Placement of `SAFE_DIR` and Refusal Log Strings
- **Problem**: Where should the constant `SAFE_DIR` (`"/safe"`) and the refusal log strings be centralized?
- **Decision**: Centralized in a shared header `kernel/taint.h` (or `kernel/param.h`), owned by Member D. The four refusal log formats are:
  - `taint: unlink refused for pid %d\n`
  - `taint: link refused for pid %d\n`
  - `taint: write refused for pid %d\n`
  - `taint: exec refused for pid %d\n`
- **Rationale**: Avoids string drift across kernel files; ensures exact compliance with invariant I4 and QA parsing scripts.
- **Alternatives Considered**: Inlining string literals at each refusal site (rejected: prone to typo divergence).
- **Affected Tasks & Modules**: D-02, B-06, C-06, C-07, C-08, D-05.
- **Verification**: Grep checks across `kernel/` to ensure identical log strings.
- **Approval Status**: PROPOSED (Plan default; frozen contract).

---

## DEC-4: Log String for `sys_open` Gate Refusal
- **Problem**: When `sys_open` refuses to open a clean file for writing or truncation by a tainted process, what log string must be emitted?
- **Decision**: Emit `taint: write refused for pid %d\n` (identical to `filewrite` refusal, per v4 D4 and Handbook Table 25.1 item X7).
- **Rationale**: Invariant I4 strictly limits the kernel to four canonical refusal log strings. Opening for write/truncation is a write-down attempt.
- **Alternatives Considered**: Inventing a `taint: open refused` string (rejected: violates invariant I4).
- **Affected Tasks & Modules**: C-08 (`sysfile.c`), tests T18/T19.
- **Verification**: Test T18/T19 log output inspection.
- **Approval Status**: PROPOSED (Plan default; frozen contract).

---

## DEC-5: Creation Mechanism for `/safe` Directory and Staged Binaries
- **Problem**: How do the `/safe` directory and its clean test binaries exist for tests T10/T12/T20 and agent demos?
- **Decision**: Created at runtime inside xv6 by a clean initialization prelude or dedicated setup test program (`mkdir("/safe", ...)`, linking/copying binaries).
- **Rationale**: `mkfs/mkfs.c` cannot create directories on the host image without complex modifications (Handbook item X13). Creating `/safe` in guest space by a clean process adheres to standard xv6 capabilities without modifying `mkfs`.
- **Alternatives Considered**: Modifying host `mkfs.c` to parse directory paths (rejected: high risk to filesystem stability).
- **Affected Tasks & Modules**: A-10, D-08, tests T10, T12, T20.
- **Verification**: Directory presence and `ls /safe` output in guest.
- **Approval Status**: PROPOSED (Marked provisional pending C-01/D-08 verification).

---

## DEC-6: Optional Hardening Gates (OH1, OH2, OH3)
- **Problem**: Whether to retain optional hardening rules OH1 (`link` refusal), OH2 (non-file `create` refusal), and OH3 (untrusted binary in `/safe` refusal).
- **Decision**: Keep OH1 (blocks attack A4) and OH3 (blocks attack A16). For OH2, non-file creation attempts by tainted processes are refused logging `taint: write refused for pid %d\n`.
- **Rationale**: Minimal complexity; closes laundering loopholes identified in the threat model. If schedule slips, OH2 and OH3 are cut first (Cut Order Tier 3).
- **Alternatives Considered**: Omitting optional hardening (rejected: weakens red-team defense in A-11/A-14).
- **Affected Tasks & Modules**: C-06, C-07, D-05, A-11.
- **Verification**: Attack scripts A4, A16 in `atest.c`.
- **Approval Status**: PROPOSED (Team hardening decision).

---

## DEC-7: Ownership of `mkfs/mkfs.c`
- **Problem**: The project handout lists `mkfs` but assigns no explicit owner in the four-member breakdown.
- **Decision**: Member C owns `mkfs/mkfs.c` if modifications are needed. Under the Option C layout (`NDIRECT 11`, 1 uint for `untrusted`, dinode size 64 bytes), `mkfs.c` is fully symbolic and zeroes new dinodes; no source edits to `mkfs.c` are required.
- **Rationale**: Handbook item X1 confirmed `mkfs.c` requires no code edits under 64-byte dinode Option C.
- **Alternatives Considered**: Assigning to Member A (rejected: Member C owns all disk and inode structures).
- **Affected Tasks & Modules**: C-01, C-02, `mkfs/mkfs.c`.
- **Verification**: `_Static_assert(sizeof(struct dinode) == 64, "dinode size")` in `fs.h`.
- **Approval Status**: CONFIRMED (Verified against pinned source).

---

## DEC-8: Repository Paths and Conventions (Item X6)
- **Problem**: Pinned repository layout, evidence scripts, issue tracking, and contribution logs must follow a uniform directory structure.
- **Decision**:
  - Architectural decisions: `CHANGES.md` at repository root.
  - Plan deviations: `PLAN_DEVIATIONS.md` at repository root.
  - Per-member contributions: `changes.md` template at root; individual records in `docs/` or `submissions/`.
  - Issue log: `logs/issues.md`.
  - Audit table: `logs/audit.md`.
  - Evidence header script: `scripts/evidence_header.sh`.
  - Post-merge checker: `scripts/post_merge_check.sh`.
  - Test runner for host unit tests: Python `pytest` or `unittest`.
  - Kernel and user source: Root `kernel/`, `user/`, `mkfs/`, `Makefile`.
- **Rationale**: Eliminates path ambiguity across test automation scripts, git operations, and gate checklists.
- **Alternatives Considered**: Submodule / external directory for xv6 (rejected: prevents direct git attribution on source files).
- **Affected Tasks & Modules**: All members, A-02, B-01.
- **Verification**: `scripts/post_merge_check.sh` and filesystem tree audit.
- **Approval Status**: PROPOSED (Ratified by Member A and Member B).

---

## DEC-9: Consolidated Instructor Clarifications (U4 through U10)
- **Problem**: Handout and execution plan contain unresolved assumptions regarding deadlines, team parameters, real LLM requirements, T17 definition, and submission packaging.
- **Decision**: Member A and Member B consolidate the open questions (U4-U10) and submit them to the course instructor on Day 1:
  - U4: Submission deadline (11-day plan vs handout 15-20 days).
  - U5: Confirmation of per-team parameters (none stated for Q3).
  - U6: Live LLM requirement (confirmed simulated/deterministic is authoritative).
  - U7: T17 test definition (handbook delete-reboot-recreate sequence).
  - U8-U10: Team zip naming convention (`Team_Name_problem_no.zip`) and Lab 9 individual upload formats.
- **Rationale**: Early resolution prevents last-minute submission re-packaging penalties.
- **Alternatives Considered**: Individual ad-hoc inquiries (rejected: causes conflicting responses).
- **Affected Tasks & Modules**: A-20, A-21, A-23, B-13, B-14, C-14, D-14.
- **Verification**: Recorded in `logs/issues.md` under open tracking rows.
- **Approval Status**: CONFIRMED (Submission draft prepared).

---

## Signatures & Consensus Record
- Member A: Aarushi Singh (`aarushisingh1137@gmail.com`) — PROPOSED / RATIFIED
- Member B: Anshika Jha (`anshikabvbvpn0822@gmail.com`) — PROPOSED / RATIFIED
- Member C: [Pending Team Ratification / Unassigned Identity]
- Member D: [Pending Team Ratification / Unassigned Identity]
