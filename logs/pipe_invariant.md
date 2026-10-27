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
- **False Negative Risk**: 0. A reader cannot consume tainted bytes without triggering the latch because `tainted_hi` marks the high-water offset of tainted data.
- **False Positive Risk**: Documented and accepted. If a clean writer writes bytes 0..9 and a tainted writer writes bytes 10..19 (`tainted_hi = 20`), a reader taking bytes 0..4 will observe `tainted_hi - nread = 20 - 0 = 20 > 0` and become tainted even though its 5 bytes were clean. This conservative over-tainting preserves system security without incurring per-byte tag memory overhead in kernel heap.
- **Concurrency & Deadlock Safety**: The pipe lock (`pi->lock`) guards both `nwrite`, `nread`, `tainted_hi`, and `has_taint`. No sleeping or scheduling occurs with un-synchronized pipe state.
