# Baseline Audit and Toolchain Verification (Task A-01)

## 1. Toolchain Verification

An environment probe was performed on the current development machine:

| Tool | Expected / Required | Detected Status on Host | Path | Compatible? | Notes |
|---|---|---|---|---|---|
| Host GCC | GCC >= 11 | GCC 13.2.0 | `/usr/bin/gcc` | YES | Host utilities (`mkfs`) compile normally. |
| GNU Make | Make >= 4.0 | GNU Make 4.4.1 | `/usr/bin/make` | YES | Standard build orchestrator. |
| Python | Python >= 3.10 | Python 3.12.3 | `/home/dkstlzk/.pyenv/shims/python3` | YES | Host test suite and automation scripts. |
| RISC-V Cross Compiler | `riscv64-unknown-elf-gcc` or `riscv64-linux-gnu-gcc` | **NOT INSTALLED** | N/A | **NO (Prerequisite Limitation)** | No cross-compiler found on host `PATH`. Sudo permissions denied by user. |
| QEMU RISC-V System Emulator | `qemu-system-riscv64` >= 7.2 | **NOT INSTALLED** | N/A | **NO (Prerequisite Limitation)** | RISC-V emulator not installed on host. |

> [!WARNING]
> The missing RISC-V compiler toolchain and QEMU emulator on this specific Linux host environment represent an external environment prerequisite limitation, strictly distinguished from source-code or architectural failure.

---

## 2. Pinned Baseline Source & Upstream Verification

1. **Pinned Upstream Commit**: `f780d7850765d74cbdcd1fff4b822d8829f7c9bd` (mit-pdos/xv6-riscv, branch `riscv`).
2. **Day-1 Source Verification**:
   - `struct dinode`: 64 bytes, 12 direct blocks (`NDIRECT 12`), `IPB = 16`, `BSIZE = 1024`.
   - `mkfs/mkfs.c`: Validates `BSIZE % sizeof(struct dinode) == 0`.
   - `ialloc` zeroes inode upon allocation; `stat.h` exposes `uint ino`.
   - Verified in `logs/G1/layout.log` matching Handbook Appendix D commands.

---

## 3. Preserved Baseline Evidence Logs

Member A executed and preserved complete baseline logs under `logs/G1/` using toolchain `riscv64-unknown-elf-gcc` 13.2.0 and `qemu-system-riscv64` 8.2.2:

1. **Build Log (`logs/G1/build.log`)**:
   - Command: `Clean build (kernel + fs.img)`
   - Artifacts generated: `kernel/kernel` (64-bit ELF), `fs.img`, `user/_*` binaries.
   - Status: PASS (Clean compile and link).
2. **Layout Log (`logs/G1/layout.log`)**:
   - Commands: Handbook Day-1 verification sequence.
   - Status: PASS (Confirmed pinned commit `f780d7850765d74cbdcd1fff4b822d8829f7c9bd`).
3. **Stock Usertests Suite (`logs/G1/baseline_usertests_cpus1.log`)**:
   - All standard unit tests (`copyin`, `copyout`, `truncate*`, `fork*`, `mem`, `sharedfd`, `linkunlink`, `bigfile`, `dirfile`, `kernmem`, etc.) passed.
   - Suite paused during long-running disk stress test (`diskfull`).
4. **Targeted Usertests Recovery (`logs/G1/baseline_usertests_slowtests_cpus1.log` & `baseline_usertests_bigdir_cpus1.log`)**:
   - `usertests diskfull`: Handled block exhaustion gracefully (`balloc: out of blocks`) -> `ALL TESTS PASSED`.
   - `usertests outofinodes`: Handled inode table exhaustion gracefully (`ialloc: no inodes`) -> `ALL TESTS PASSED`.
   - `usertests bigdir`: Directory stress test verified -> `ALL TESTS PASSED`.
