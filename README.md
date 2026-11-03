# xv6 Taint-Tracking System & Agentic Lab

**IIT Jodhpur — CSL3030 Operating Systems Lab — Project 3 (Q3)**

This repository contains the complete implementation through **Day 4** of the taint-tracking extension to `xv6-riscv` and the accompanying host-side Agentic Lab test framework.

---

## 1. Project Overview & Architecture

The system implements mandatory information-flow security inside the xv6 kernel to defend against prompt injection and malicious tool execution in agentic AI workloads:

- **Process Taint Lifecycle**: Processes track an explicit `tainted` flag in `struct proc`, initialized to `0`, inherited across `fork()`, and preserved monotonically across `exec()`.
- **Filesystem Persistence**: The on-disk inode (`struct dinode`) is extended with `uint untrusted` while preserving the exact 64-byte disk layout (`NDIRECT 11`, `_Static_assert(sizeof(struct dinode) == 64)`).
- **Taint Latching**: Reading positive bytes (`> 0`) from an untrusted inode latches process taint permanently.
- **Write-Down Prevention**: Tainted processes are strictly prevented from writing to or truncating clean regular files (enforced in `filewrite` and `sys_open`).
- **Filesystem Hardening**: Tainted processes cannot unlink or link files (`sys_unlink`, `sys_link`). Newly created files by tainted processes are born untrusted on disk (`create()`).
- **Execution Containment**: Tainted processes cannot execute binaries outside `/safe` (`kexec`). Executing directories (`/safe`, `/`) or untrusted binaries inside `/safe` is rejected.
- **Pipe Watermarking**: Inter-process communication pipes track a high watermark (`tainted_hi`, `has_taint`) per-byte to eliminate false negatives when multiple writers share a pipe.
- **Canonical Refusal Logs**: Every kernel refusal emits exactly one canonical log line:
  - `taint: unlink refused for pid %d\n`
  - `taint: link refused for pid %d\n`
  - `taint: write refused for pid %d\n`
  - `taint: exec refused for pid %d\n`
- **Host Agentic Lab**: Python agent (`agent_lab/xv6agent/`) with Layer-1 action validation, QEMU bridge with unanchored lookbehind sentinel framing, persistent-disk mode, and frozen 8-field trace generation.

---

## 2. Repository Structure

```text
os-project/
├── kernel/                  # xv6-riscv kernel source
│   ├── proc.c / proc.h      # Process allocation, free, and taint inheritance
│   ├── fs.c / fs.h          # Inode 64B layout, untrusted disk persistence
│   ├── file.c / file.h      # fileread taint latch and filewrite write-down gate
│   ├── sysfile.c            # sys_marktaint, create, sys_open, sys_unlink, sys_link
│   ├── exec.c               # kexec pre-lock and post-lock execution containment
│   ├── pipe.c               # Pipe watermark tracking (tainted_hi, has_taint)
│   ├── taint.c / defs.h     # in_safe() directory lookup and ip_untrusted()
│   └── syscall.* / sysproc.c# System calls: sys_marktaint (23), sys_istainted (24), sys_hello (25)
├── user/                    # xv6 user programs and test suites
│   ├── tlib.h               # Unified test harness macros (CHECK, INCONCLUSIVE, SUMMARY)
│   ├── hello.c              # Hello syscall verification utility
│   ├── mark.c               # CLI to mark files untrusted
│   ├── ttest.c              # Member B process & syscall tests (T0, T1, T3, T4, T11a)
│   ├── ftest.c              # Member C filesystem tests (T5, T9, T11b, T13, T16, T18, T19)
│   ├── ptest.c              # Member D pipe & exec containment tests (T6, T7, T8, T10, T12, T14, T20, stress)
│   └── atest.c              # Member A red-team adversarial attacks (A1..A4, A8, A9, A16..A18)
├── agent_lab/               # Host-side agentic testing lab (Member A)
│   ├── xv6agent/            # config, tools (validate_action), planner, bridge, agent
│   ├── tests/               # test_validator.py (20 unit tests), test_bridge.py (10 tests)
│   ├── demo_agent.py        # Layer-1 prompt injection demonstration
│   ├── demo_kernel.py       # Layer-2 kernel defense demonstration (Runs A, B, C)
│   └── make_lab_image.sh    # Script to stage exercise files on disk image
├── artifacts/               # Team replication deliverables
│   └── day1-day4-commits/   # 47 chronological patches, COMMIT_MANIFEST.md, REPLICATION_GUIDE.md
├── docs/                    # Architectural documents and audit reports
│   ├── BASELINE_AUDIT.md    # Initial toolchain and upstream baseline audit
│   ├── DECISIONS.md         # Summary of architectural decisions DEC-1 through DEC-9
│   ├── DAY1_DAY4_IMPLEMENTATION_AUDIT.md # Initial task-by-task status matrix
│   └── DAY1_DAY4_FINAL_REPORT.md         # Final comprehensive completion and evidence report
├── logs/                    # Audit tables and execution evidence logs
│   ├── audit.md             # 3 cross-member security audit passes for denial obligations
│   ├── issues.md            # Project defect and question tracking register
│   ├── pipe_invariant.md    # Pipe watermark invariant and safety analysis
│   ├── G1/                  # Gate G1 baseline build, layout, and hello smoke evidence
│   ├── G2/                  # Gate G2 validator and two-boot persistence evidence
│   ├── G3/                  # Gate G3 bridge and Layer-1 agent trace evidence
│   └── G4/                  # Gate G4 test suites (ttest, ftest, ptest, int1-int7, demo_kernel)
├── scripts/                 # Automation scripts
│   ├── evidence_header.sh   # Standardized evidence header generator
│   └── post_merge_check.sh  # Clean build, boot, and smoke checker
├── CHANGES.md               # Team architectural decision freeze (DEC-1..DEC-9)
├── changes.md               # Individual contribution record template and logs
├── PLAN_DEVIATIONS.md       # Approved plan deviation log (DEV-01)
└── Makefile                 # Build system wiring for taint.o, stubs, and fs.img
```

---

## 3. Team Member Allocation & Roles

- **Member A** (`Aarushi Singh`): Host Agentic Lab, QEMU bridge, Makefile build wiring, red-team attack suite (`atest.c`), packaging, and Gate G4.
- **Member B** (`Anshika Jha`): Syscall infrastructure (`sys_hello`, `sys_istainted`), process taint lifecycle, file read/write gates, `ttest.c`, coordination, and Gate G1.
- **Member C**: Inode layout redesign (`NDIRECT 11`, 64B dinode), disk persistence across reboot, `sys_marktaint`, `create()` born untrusted, `ftest.c`, and Gate G2.
- **Member D**: Pipe invariant design and watermark tracking (`tainted_hi`), directory containment (`in_safe`), exec containment (`exec.c`), `ptest.c`, and Gate G3.

---

## 4. How to Run Tests

### Host-side Unit & Integration Tests (Python 3):
```bash
# 1. Action validator unit tests (20 tests)
python3 -m unittest agent_lab/tests/test_validator.py

# 2. QEMU bridge unit tests (10 tests)
python3 -m unittest agent_lab/tests/test_bridge.py

# 3. Layer-1 Prompt Injection Demo (emits 8-field trace)
PYTHONPATH=. python3 agent_lab/demo_agent.py

# 4. Layer-2 Kernel Refusal Demo (Runs A, B, C)
python3 agent_lab/demo_kernel.py
```

### Guest xv6 Tests (in QEMU):
```bash
make clean && make qemu
# Inside xv6 shell:
$ hello     # Verifies sys_hello (sum = 10)
$ ttest     # Verifies process taint & fork inheritance (5 tests)
$ ftest     # Verifies filesystem persistence & refusals (9 tests)
$ ptest     # Verifies pipe watermark & exec containment (8 tests)
$ atest     # Verifies red-team attack defenses (9 attacks)
```

---

## 5. Replicating Commits to the Official Team Repository

All changes through Day 4 are packaged as 47 chronological patches in `artifacts/day1-day4-commits/`.
To replicate:
1. Refer to [REPLICATION_GUIDE.md](file:///home/dkstlzk/Desktop/sem%205/OS/OS-project/os-project/artifacts/day1-day4-commits/REPLICATION_GUIDE.md).
2. Inspect the commit sequence in [COMMIT_MANIFEST.md](file:///home/dkstlzk/Desktop/sem%205/OS/OS-project/os-project/artifacts/day1-day4-commits/COMMIT_MANIFEST.md).
3. Apply via `git am artifacts/day1-day4-commits/*.patch`.
