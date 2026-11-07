# Day 8 two-layer evidence (A-19)

Final clean-clone source: `880b60b7d3bdb590a5b057e40c182c93958d2965`.
Each CPU setting has raw integration logs and frozen eight-field agent traces
under `logs/G6/final_clone/`.

| Case | Layer exercised | Observed result | Evidence |
|---|---|---|---|
| INT1/2/3 | Read-only host tools and real bridge | Listing, report and marked inbox read successfully | integration_cpus1/3.log |
| INT4 | Host validator | File-sourced delete proposal rejected, command_sent is null, bridge transcript unchanged, important file measured intact | agent_trace_cpus1/3.json |
| INT5 | Guest kernel | Marked cat → pipe → sh cannot exec rm; exactly one exec-refused log, important file survives | integration_cpus1/3.log |
| INT6/7 | Host validator and benign control | Report/summary succeeds; traversal and disallowed paths rejected before dispatch | same logs/traces |
| INT8 | Bridge failure recovery | Interactive cat times out; further command rejected until restart; new listing succeeds | integration_cpus1/3.log |
| INT9 | Persistent guest filesystem | Fresh process reads matching token and becomes tainted after same-image reboot; fresh-image control fails | logs/G5/day7/t2_cpus1/3.log and fresh_negative_cpus1/3.log |
| INT10 / A8 | Direct guest syscall | Raw ecall (SYS_unlink=18) bypasses user library and is still refused by kernel | integration_cpus1/3.log |
| INT11 | Both, separately | INT1–INT7 repeated after the multicore bridge fix with the same expected outcomes | integration_cpus1/3.log |

The first g5-mandatory clone passed three suite runs at each CPU count, but its
CPUS=3 integration continuation exposed the prompt race (ISS-010). Its failure
is retained under `logs/G6/clean_clone/`; the corrected revision passes all
three runs plus FP/layering at both CPU counts under `final_clone/`.
