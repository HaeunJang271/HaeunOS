# Figure F4 — Experimental evidence coverage

**Matches:** `docs/r44-experimental-statistics-audit.md` (no new numbers).

```mermaid
flowchart LR
  subgraph Covered["Covered / recorded"]
    R41[R4.1 standard suites]
    R42P[R4.2 120 descriptive runs]
    R42F[R4.2 30 profile recommendations]
    R43A[R4.3 RR↔FIFO apply + 61/61]
    R43S[R4.3 reject / rollback ID]
    R432[R4.3.2 same-host worktree chain exit 0]
  end
  subgraph Open["Explicitly open"]
    T3[Third-party reproduction NOT TESTED]
    PC[30-cell live APPLY NOT_TESTED_per_cell]
    INF[Inferential stats NOT PERFORMED]
    SMP[SMP / HW atomicity NOT CLAIMED]
  end
```

**Caption.** Coverage map of existing campaigns versus open limitations. No invented results.
