# R3.1 Timestamp Safety

FIFO uses `ready_since` (last READY entry) with signed modular order.

Reliable only when pairwise separation is **strictly less than** `2^31`.
Separation **exactly** `2^31` ⇒ FAIL (ambiguous).

Snapshot checks:

- Empty eligible set → NOT_APPLICABLE
- Single candidate → PASS pairwise; external bound UNKNOWN
- Multi: no half-range pairs → pairwise PASS; **lifetime `<2^31` unbound → UNKNOWN**

R3.1 does not change the FCFS algorithm and does not add a sequence counter.
