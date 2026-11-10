# Gate G7b Release Evidence Index (Task X-12 / X-13)

**Gate**: G7b (`v1.0-submission`)  
**Milestone**: M8 Wave 4–6 (Final Release Gate)  
**Lead**: Member D  
**Status**: PASS  

---

## 1. Full Release-Candidate Regression Results

Executed on the final frozen release commit across both uniprocessor (`CPUS=1`) and multiprocessor (`CPUS=3`) configurations:

| Test Suite / Component | Evidence File | Execution Condition | Result |
| :--- | :--- | :--- | :--- |
| **Syscall & Process Suite (`ttest`)** | `ttest_cpus1.log`, `ttest_cpus3.log` | Fresh guest kernel, positive/zero-byte reads, fork inheritance | **PASS** (0 fail) |
| **Filesystem Suite (`ftest`)** | `ftest_cpus1.log`, `ftest_cpus3.log` | Born untrusted, open/unlink/link write gates, T13 inode reuse | **PASS** (0 fail, confirmed reuse) |
| **Pipe & Exec Suite (`ptest`)** | `ptest_cpus1.log`, `ptest_cpus3.log` | Watermark propagation, `/safe` containment, exec taint | **PASS** (0 fail) |
| **Adversarial Attack Suite (`atest`)**| `atest_cpus1.log`, `atest_cpus3.log` | Attacks A1–A4, A8, A9, A16–A18; raw assembly `ecall` check | **PASS** (9 blocked, mechanisms logged) |
| **False-Positive Controls (FP-1..5)** | `fp_1_cpus1.log` through `fp_5_cpus3.log` | Marked `/inbox.txt` inputs: console, pipes, redirection, shell | **PASS** (0 bugs) |
| **Host Integration Suite (INT1..11)** | `integration_cpus1.log`, `cpus3.log` | Real `QemuBridge`, agent dispatcher, prompt injection defense | **PASS** (10/10 checks) |
| **Host Unit Test Suite** | `host_tests.log` | Python unittest discovery in `agent_lab/tests` | **PASS** (35 tests OK) |

---

## 2. Release Packaging & Verification (Task A-23)

- **Team Release Archive**: `submissions/team.zip`
- **Clean Unpack Build Verification**: Unpacked `submissions/team.zip` over a fresh clone of pinned upstream xv6-riscv (`f780d7850765d74cbdcd1fff4b822d8829f7c9bd`), executed `make clean && make -j4 kernel/kernel fs.img`, build completed with exit code 0.
- **Individual Submission Archives**: `submissions/member_A.zip`, `member_B.zip`, `member_C.zip`, `member_D.zip` all verified byte-for-byte against Git blobs via `scripts/verify_individual.sh`.

---

## 3. Final Sign-offs (Task X-13)
- Member A (Agent, Bridge, Build, Red Team): Verified & Signed.
- Member B (Syscalls, Process Taint, Integration): Verified & Signed.
- Member C (Filesystem Persistence, Inode Allocation, Audits): Verified & Signed.
- Member D (Pipes, Exec Containment, Concurrency Proofs): Verified & Signed.
- **Final Release Tag**: `v1.0-submission`.

