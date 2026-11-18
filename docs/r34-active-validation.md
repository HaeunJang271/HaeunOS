# R3.4 Active Validation

R3.1 remains authoritative.

With `ready_seq` unique and `wrap==0`, FIFO timestamp/ordering check returns **PASS** (bounded).

`wrap!=0` → UNKNOWN (reject). Duplicate seq → FAIL. Legacy ready_since-only → UNKNOWN (unchanged).

WARN is never promoted to PASS.
