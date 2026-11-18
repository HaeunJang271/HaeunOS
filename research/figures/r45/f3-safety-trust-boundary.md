# Figure F3 — Safety boundary and trust separation

**Matches:** `docs/r44-safety-invariant-audit.md`, `r40-safety-model.md`.

```mermaid
flowchart TB
  subgraph Untrusted_for_auth["Must not authorize alone"]
    OPT[Optimizer / planner / feedback]
  end
  subgraph Gate["Host safety layer"]
    PVAL[pval snapshot + invariants]
    PRE[prepare UNKNOWN≠PASS]
    CMT[commit at between-step convention]
  end
  subgraph Guest_state["Guest mutable policy"]
    WID["0xA474 policy ID"]
  end
  OPT -->|recommendation only| PRE
  PVAL --> PRE
  PRE -->|authorized| CMT --> WID
  Note1[Single-thread interpreter assumption]
  Note2[Not SMP / not HW atomicity]
  Note3[Not full VM rollback]
```

**Caption.** Trust separation: recommendation ≠ authorization. Safe-point semantics are interpreter conventions, not hardware atomicity.
