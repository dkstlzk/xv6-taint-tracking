# Pipe Invariant and Watermark Design Note (Task D-01)
Author: Member D

## 1. Safety Invariant
A reader consuming from a pipe becomes tainted if and only if it consumes a byte at or before the latest tainted write position that is still unread in the circular buffer.

Formally:
- Each write of length $L$ by process $P$ advances the pipe's absolute byte sequence counter `nwrite` by $L$.
- If $P$ is tainted (`P->tainted == 1`), `has_taint = 1` and `tainted_hi = nwrite`.
- When a reader $R$ consumes bytes starting at `nread`, `R` is tainted if:
  `has_taint && (int)(tainted_hi - nread) > 0`
  computed before the copy.

## 2. Conservatism and Trade-offs
- **False-negative argument**: while tainted bytes remain unread, the latest tainted offset is at most PIPESIZE (512) bytes ahead of nread. The signed difference is positive even across uint wrap. Every copied tainted byte updates the watermark while holding the same lock as the reader. Testing provides bounded evidence, not a claim of zero risk under every execution.
- **False Positive Risk**: Documented and accepted. If a clean writer writes bytes 0..9 and a tainted writer writes bytes 10..19 (`tainted_hi = 20`), a reader taking bytes 0..4 will observe `tainted_hi - nread = 20 - 0 = 20 > 0` and become tainted even though its 5 bytes were clean. This conservative over-tainting preserves system security without incurring per-byte tag memory overhead in kernel heap.
- **Concurrency & Deadlock Safety**: The pipe lock (`pi->lock`) guards both `nwrite`, `nread`, `tainted_hi`, and `has_taint`. No sleeping or scheduling occurs with un-synchronized pipe state.

## Day 6 decision — D-12

Retain the watermark design. The assigned B review in audit pass #2 checks the
same-lock update/read invariant. Day 5 ran 20 CPUS=3 trials (seed 5101); Day 6 ran
60 more (seed 6101), with zero observed false negatives and no unexplained
intermittent failure. Each trial forks both writers while the coordinator is
clean, writes 1024 C and 1024 T bytes concurrently (forcing full-pipe sleeps),
reads randomized partial lengths, checks writer exit statuses and byte totals,
and records the entire observed ordering. T8 confirms the predicted conservative
clean-prefix false positive. See `logs/G5/day5/stress20_cpus3.log` and
`logs/G5/day6/stress60_cpus3.log`.

Signed comparison is unambiguous for outstanding tainted bytes (distance ≤512).
Since has_taint remains set, a very stale watermark after over 2^31 subsequent
clean bytes can conservatively taint again; no multi-gigabyte/wrap campaign is
claimed. Taint remains process-wide and permanent. The fallback is per-pipe
ever-tainted state; apply `artifacts/pipe_per_pipe_fallback.patch` only if a later
campaign violates the decision criteria. It deliberately adds false positives.
