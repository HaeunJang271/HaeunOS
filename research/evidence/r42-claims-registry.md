# R4.2 Claims Registry

| ID | Claim | Category | Evidence | Limits |
|----|-------|----------|----------|--------|
| C1 | RR/FIFO selectable via policy abstraction | mechanism | A+B r20/r30 | 2 policies |
| C2 | Validation-gated txn at between-step SP | mechanism+test | A+B r31–r34 | no SMP |
| C2.1 | UNKNOWN ≠ PASS | contract | A+B | — |
| C2.2 | Rollback = policy ID | mechanism | A+B | not full VM |
| C3 | Host opt evaluates candidates, recommends | mechanism+runtime | A+B+C R4.2 | host-only |
| C3.1 | SHADOW default; no direct kernel write by opt | mechanism | A+B | — |
| C4 | Preset campaigns / hashes / replay | runtime | C; separate-dir C | 3rd party F |
| C5 | Recommendation verify / multi-profile | runtime | C 6×5 | apply/cell F |
| C5.1 | All 5 profiles SELECT RR on catalog n=10 | observed | C | catalog-bound |
| P1 | RR lower ready_wait than FIFO on 6 wl | descriptive | C | not E |
| P2 | Inferential superiority | — | F | not claimed |

JSON: `r42-claims-registry.json`.
