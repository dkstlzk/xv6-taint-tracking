# Measured limits through Day 8

- Process taint is permanent and coarse. A shell that reads untrusted input
  remains tainted; subsequent exec outside /safe is refused (FP-5).
- The pipe watermark conservatively taints clean bytes before an unread marked
  byte (T8). Dedicated campaigns contain 20 + 60 + 60 + 60 + 60 trials across
  Days 5–8, plus a final 60-trial clean-source confirmation (320 total),
  including a separate host-load session, with no false negatives
  observed. This is bounded testing, not exhaustive concurrency verification.
  See `logs/pipe_invariant.md` for the signed-distance argument and stale-mark
  limitation after more than 2^31 clean bytes. No counter-wrap run is claimed.
- /safe membership is based on direct inode membership, not a pathname prefix.
  Clean users can provision it, including hard links. This is a teaching policy,
  not a general sandbox against a malicious clean administrator.
- A clean shell opens/truncates a redirect target before cat reads its marked
  input. Thus FP-3's write is refused but the target may already be empty.
  New files explicitly created after taint are born untrusted and writable (FP-4).
- NDIRECT is 11: maximum file data is 267 1024-byte blocks rather than 268.
  Both on-disk inode size (64 bytes) and IPB (16) remain unchanged. Rebuild old
  images after layout changes. T13 proves reuse by inode equality; T17 makes no
  reuse claim.
- Normal persistence phases sync and verify the same image hash/token across
  reboots. Optional kill-reboot tests omit that explicit sync and SIGKILL QEMU
  after the mark command returns. They do not simulate every log crash window
  or physical storage power failure.
- The deterministic planner and simulated gullible planner exercise actual
  guest input and validation. No external LLM provider is integrated (optional
  A-20 is Day 9). Agent rejection is Layer 1 evidence; guest/raw-ecall refusal
  is independent Layer 2 evidence.
- The bridge expects the stock xv6 shell protocol, allows at most 80 command
  characters, waits for a random sentinel and the following prompt, and treats
  timeouts as desynchronization. Sentinel completion alone is not exit status;
  test runners check actual result lines. Commands reading interactive stdin
  intentionally time out until restart.
- All assisted commits use the user-specified pmakde identity. Member labels
  express assigned ownership. Human peer reviews, individual contribution
  certification, viva demonstrations, and signatures cannot be inferred from
  those labels. They remain team responsibilities.
- G6 requires Day 9's final audit and review. G7/report/rehearsal/final packaging
  are Day 10–11 work. No such gate is claimed by this Day 8 implementation.
