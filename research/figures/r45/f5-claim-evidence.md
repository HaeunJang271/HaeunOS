# Figure F5 — Claim / evidence relationship

**Matches:** `docs/r44-claim-evidence-matrix.md` classes A/B/C.

```mermaid
flowchart TB
  subgraph A["A Can claim"]
    A1[Pipeline evaluate→recommend]
    A2[Authority separation]
    A3[UNKNOWN≠PASS prepare]
    A4[Prepare ≠ commit]
    A5[Recorded 61/61 mirror apply]
  end
  subgraph B["B Qualified"]
    B1[Autonomy = eval not unattended APPLY]
    B2[Safe point = between-step convention]
    B3[Rollback = policy ID]
    B4[Descriptive ready_wait catalog]
    B5[Same-host worktree repro]
  end
  subgraph C["C Cannot claim"]
    C1[Statistical superiority]
    C2[Fairness improvement]
    C3[Third-party success]
    C4[Full-system rollback]
    C5[SMP / HW atomicity]
  end
  EV[Freeze sources + R4.2/R4.3 artifacts + R4.4 audit] --> A
  EV --> B
  EV -.->|insufficient or contradicted| C
```

**Caption.** Publication claims must stay in A/B. Class C items are withheld despite proximity to the topic.
