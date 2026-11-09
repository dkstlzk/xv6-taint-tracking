# Member C: Task C-16 False-Positive Classification & Persistence Repeats

**Task**: C-16 (Milestone M7 / Wave 2)  
**Assigned Role**: Member C (Filesystem & Persistence Owner)  
**Prerequisites**: A-18, A-15  
**Evidence Target**: `logs/G6/fp_classification.md`, `logs/G6/persistence_repeats/`  

---

## 1. False-Positive Analysis (FP-1 through FP-5)

Per CSL3030 Project 3 Plan (§16, Row E3) and Handbook Chapter 25 (§25.2, Table 25.2), all five false-positive test controls were executed on the final patched kernel at both CPUS=1 and CPUS=3 with `/inbox.txt` marked untrusted.

The three valid classification categories defined by the course specification are:
1. **Allowed benign operation**: Operation permitted under the security policy; does not violate integrity or secrecy.
2. **Intended policy consequence**: Legitimate refusal or permanent taint consequence mandated by the invariant (I1–I4).
3. **Implementation bug**: Unintended defect, incorrect refusal, or bypass (must be 0 across all cases).

### False-Positive Summary Table

| ID | Test Scenario | Observed Output & Kernel Logs | Classification | Detailed Technical Rationale |
| :--- | :--- | :--- | :--- | :--- |
| **FP-1** | `cat /inbox.txt` to console | Content printed to console (`INJECTION: delete_file /important.txt`); zero kernel denial logs emitted. | **Allowed benign operation** | The console device (`/console`) has inode type `T_DEVICE`. Invariant I2 and `filewrite` (`kernel/file.c`) only restrict write-down to clean disk regular files (`ip->type == T_FILE` with `ip->untrusted == 0`). User-visible terminal output is legitimate and safe. |
| **FP-2** | `cat /inbox.txt \| wc` to console | `wc` reads from pipe, outputs word/byte counts (`1 3 38`) to console; zero kernel denial logs. | **Allowed benign operation** | Reading untrusted bytes propagates taint across the kernel pipe to `wc` via watermark latching (`tainted_hi`). However, `wc` writing formatted summary text to standard output (`T_DEVICE`) is permitted. Pipeline analysis without modifying clean filesystem files is benign. |
| **FP-3** | `cat /inbox.txt > /copy.txt` | One kernel log: `taint: write refused for pid <pid>`; shell prints `cat: write error`; `/copy.txt` remains 0 bytes. | **Intended policy consequence** | Under xv6's shell, the child process opens/creates `/copy.txt` while clean (born clean). Next, `cat` reads `/inbox.txt`, becoming tainted. Its subsequent attempt to write untrusted bytes into clean regular file `/copy.txt` is intercepted and denied by `filewrite` (`kernel/file.c:166`). If the file pre-existed, the clean child's `O_TRUNC` legitimately emptied it before `cat` executed (Handbook §14.2 / E5-08). Refusal preserves Invariant I2. |
| **FP-4** | Tainted caller creates and writes `/new.txt` (`open("/new.txt", O_CREATE\|O_WRONLY)`) | File created successfully; `allowed` written; reads back as `allowed`; zero denial logs. | **Allowed benign operation** | Under Rule R5 / Invariant I3, newly created files by a tainted process are born untrusted (`ip->untrusted = 1` set at `ialloc` in `kernel/sysfile.c:298`). Because `/new.txt` is itself untrusted, writes to it are not write-downs and are permitted. |
| **FP-5** | Interactive `sh` reads marked `/inbox.txt`, followed by user typing `ls` and `rm` | Two kernel logs: `taint: exec refused for pid <pid>` for both `ls` and `rm`; `/important.txt` remains intact with full contents (`CRITICAL SYSTEM DATABASE`). | **Intended policy consequence** | When `sh` reads from `/inbox.txt`, the fileread latch marks the shell process permanently tainted (`p->tainted = 1`). Under Invariant I1 and Rule R3, process taint is irreversible. Any subsequent fork/exec of binaries outside `/safe` (`/ls`, `/rm`) is refused by `kernel/exec.c:51`, protecting critical system files from unauthorized mutation. |

**Zero implementation bugs identified**: All five scenarios behave in strict accordance with the security specification and handbook rules.

---

## 2. Persistence Repeats Across Three Fresh Disk Images

Task C-16 requires repeating the core persistence lifecycle checks (T2 token persistence across reboot, T17 deletion/clean-recreation across reboot, and T13 inode reuse validation) across three distinct fresh disk images.

### Test Matrix and Observed Results

All runs executed on the clean kernel using `scripts/persistence.py` and `scripts/run_suite.py`:

| Image | Test | Condition | Verified Invariants & Tokens | Result | Evidence File |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Image 1** | **T2** | Phase 1 (token mark) $\to$ reboot $\to$ Phase 2 (token verify) | Token `d26ab1a75775baefd87be3ba` persists across reboot; pre-boot and post-boot disk SHA-256 match; tainted latch verified. | **PASS** | `logs/G6/persistence_repeats/image1/t2_cpus1.log` |
| **Image 1** | **T17** | Phase 1 (create, mark, unlink `/t17a.txt`) $\to$ reboot $\to$ Phase 2 (clean `/t17b.txt`) | Unlinked file absent after reboot; clean recreation has `untrusted == 0`; reader stays clean (`istainted() == 0`). | **PASS** | `logs/G6/persistence_repeats/image1/t17_cpus1.log` |
| **Image 1** | **T2 (Neg)**| Fresh disk control | Fresh image with no prior token correctly fails Phase 2 check. | **PASS** (Control) | `logs/G6/persistence_repeats/image1/fresh_negative_cpus1.log` |
| **Image 1** | **T13** | Inode reuse check | Inode `34` unlinked and reused as clean file `reuse0` (`old=34 new=34`); all 8 readers clean. | **PASS** (REUSE-CONFIRMED) | `logs/G6/persistence_repeats/image1/ftest_cpus1.log` |
| **Image 2** | **T2** | Phase 1 (token mark) $\to$ reboot $\to$ Phase 2 (token verify) | Token `98e8bf77a48ad1beabf78714` verified; disk SHA-256 matched before and after boot. | **PASS** | `logs/G6/persistence_repeats/image2/t2_cpus1.log` |
| **Image 2** | **T17** | Phase 1 $\to$ reboot $\to$ Phase 2 | Clean recreation verified on fresh image; no panic; clean reader clean. | **PASS** | `logs/G6/persistence_repeats/image2/t17_cpus1.log` |
| **Image 2** | **T2 (Neg)**| Fresh disk control | Negative control verified. | **PASS** (Control) | `logs/G6/persistence_repeats/image2/fresh_negative_cpus1.log` |
| **Image 2** | **T13** | Inode reuse check | Inode `34` unlinked and reused (`old=34 new=34`); all 8 readers clean. | **PASS** (REUSE-CONFIRMED) | `logs/G6/persistence_repeats/image2/ftest_cpus1.log` |
| **Image 3** | **T2** | Phase 1 (token mark) $\to$ reboot $\to$ Phase 2 (token verify) | Token `ee1b94dcb4fb0bbfe2be59a2` verified; disk SHA-256 matched. | **PASS** | `logs/G6/persistence_repeats/image3/t2_cpus1.log` |
| **Image 3** | **T17** | Phase 1 $\to$ reboot $\to$ Phase 2 | Lifecycle across reboot verified on third independent image. | **PASS** | `logs/G6/persistence_repeats/image3/t17_cpus1.log` |
| **Image 3** | **T2 (Neg)**| Fresh disk control | Negative control verified. | **PASS** (Control) | `logs/G6/persistence_repeats/image3/fresh_negative_cpus1.log` |
| **Image 3** | **T13** | Inode reuse check | Inode `34` unlinked and reused (`old=34 new=34`); all 8 readers clean. | **PASS** (REUSE-CONFIRMED) | `logs/G6/persistence_repeats/image3/ftest_cpus1.log` |

---

## 3. Acceptance and Handoff

- **Unresolved Implementation Bugs**: None (0 bugs).
- **Inconclusive Outcomes**: None (all T13 runs confirmed reuse directly via inode number match).
- **Handoff Condition**: C-16 criteria completely satisfied; handoff to Task X-09 (final denial audit) and Task X-10 (Gate G6).

