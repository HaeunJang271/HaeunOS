# Figure F2 — Evaluation-to-application pipeline

**Matches:** R4.4 architecture table; `opt_cli` SHADOW/APPLY; `reconfig_txn` prepare≠commit.

```mermaid
flowchart TD
  A[Workload HXF] --> B[Guest schedule + telem]
  B --> C[Host evaluate]
  C --> D[Recommend policy 0 or 1]
  D --> E{Mode}
  E -->|SHADOW default| F[Report only]
  E -->|APPLY explicit| G[haeun_recfg_prepare]
  G -->|UNKNOWN or FAIL| H[Reject: no policy change]
  G -->|PASS gates| I[haeun_recfg_commit]
  I --> J[Active policy word]
  I -.->|FI / failure| K[Rollback policy ID only]
```

**Caption.** SHADOW is default. APPLY is opt-in. Prepare is not commit. Rollback restores policy ID only.
