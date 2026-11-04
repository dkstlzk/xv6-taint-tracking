# Commit Replication and Patch Application Guide

This guide describes how to reproduce the complete Day 1–Day 4 implementation history from this dummy repository (`os-project`) into the official team repository.

---

## 1. Prerequisites and Baseline Verification

Before applying any patches, ensure the target repository is at the pinned baseline:
- **Upstream Commit**: `f780d7850765d74cbdcd1fff4b822d8829f7c9bd` (mit-pdos/xv6-riscv, branch `riscv`).
- Verify via:
  ```bash
  git rev-parse HEAD
  git status
  ```
- Ensure working directory is clean.

---

## 2. Inspecting Patches

All 47 patches are exported in chronological order inside `artifacts/day1-day4-commits/`:
```bash
# View patch contents and diff stat
git apply --stat artifacts/day1-day4-commits/0001-*.patch

# Check if patch applies cleanly without errors
git apply --check artifacts/day1-day4-commits/0001-*.patch
```

---

## 3. Applying Commits Sequentially

To preserve exact member commit messages, author metadata, and chronological ordering, use `git am`:

```bash
git am artifacts/day1-day4-commits/*.patch
```

Alternatively, to apply commit-by-commit:
```bash
for patch in artifacts/day1-day4-commits/*.patch; do
    echo "Applying $patch..."
    git am "$patch" || { echo "Conflict on $patch"; break; }
done
```

---

## 4. Conflict Resolution Strategy

If the destination repository has diverged:
1. Identify the conflicting file using `git status`.
2. Inspect the `.rej` reject file or 3-way merge markers (`git am -3`).
3. Note that `kernel/fs.h` and `kernel/file.h` introduce `NDIRECT 11` and `uint untrusted`, which affects `struct dinode` and `struct inode`. Ensure no duplicate field additions occur.
4. Continue with `git am --continue` or abort with `git am --abort`.

---

## 5. Verification After Replication

1. **Host-Side Tests**:
   ```bash
   python3 -m unittest agent_lab/tests/test_validator.py
   python3 -m unittest agent_lab/tests/test_bridge.py
   PYTHONPATH=. python3 agent_lab/demo_agent.py
   python3 agent_lab/demo_kernel.py
   ```
2. **Build and Image Verification** (when RISC-V toolchain is available):
   ```bash
   make clean && make
   make qemu
   ```
3. **Verify Milestone Tags**:
   - `m1-plumbing` (Commit at Gate G1)
   - `m2-vertical` (Commit at Gate G3)
   - `m3-ai-ready` (Commit at Gate G4)
