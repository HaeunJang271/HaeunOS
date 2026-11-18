# Figure F1 — HaeunOS AROS system architecture

**Matches:** freeze `386c3a1…`, `docs/r40-system-architecture.md`, R4.4 architecture audit.  
**Editable source:** Mermaid.

```mermaid
flowchart LR
  subgraph Guest["Guest HAEUN-32"]
    WL[HXF workloads]
    SCH[Scheduler RR / preemptive FIFO]
    TEL[Telemetry + ready_seq]
    POL["Policy word 0xA474"]
    WL --> SCH
    SCH --> TEL
    POL --> SCH
  end
  subgraph Host["Host research tooling"]
    MON[Monitor / pval snapshot]
    PL[Planner / analyzer]
    OPT[Optimizer recommend]
    REP[SHADOW report only]
    SAF[Safety prepare]
    TXN[Commit / rollback ID]
  end
  TEL -->|read-only snapshot| MON
  MON --> PL --> OPT
  OPT -->|SHADOW default| REP
  OPT -->|APPLY opt-in| SAF
  SAF -->|prepare then| TXN
  TXN -->|write policy ID only| POL
```

**Caption.** Host tooling evaluates guest telemetry and may recommend a policy. SHADOW (default) ends in host report with no `0xA474` write. APPLY is explicit opt-in and alone enters prepare→commit. Recommendation is not authorization; rollback (when used) restores policy ID only.
