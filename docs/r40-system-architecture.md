# R4.0 System Architecture (AROS)

Working title: **HaeunOS AROS: An Autonomous Runtime Policy Optimization Framework for Safe Operating System Reconfiguration**

## Logical pipeline

```mermaid
flowchart TD
  WL[Guest Workload HXF] --> SCH[Guest Scheduler]
  SCH --> TEL[Guest Telemetry PCB]
  TEL --> MON[Host Monitor / Telem Inspect]
  MON --> AN[Analyzer / Planner]
  AN --> OPT[Optimization Engine R3.6]
  OPT --> REC[Recommendation]
  REC --> SAF[R3.1 Safety Validator]
  SAF --> TXN[R3.2 Transaction]
  TXN --> POL[Active Policy Word 0xA474]
  POL --> SCH
  TEL --> FB[R3.5 Feedback]
  FB --> OPT
  RB[R3.7 Robustness] --> OPT
```

## Component placement

| Stage | Runs in |
|-------|---------|
| Workload / scheduler / telem / ready_seq | Guest kernel |
| Monitor, planner, optimize, feedback, robust | Host research tooling |
| Safety snapshot/validate | Host (reads guest RAM) |
| Policy commit | Host writes `0xA474` via R3.2 only |
| VM step / safe point | Host interpreter |

## Safety boundary

Optimization **must not** write PCB/READY/PTBR/timer.  
R3.1 authoritative; UNKNOWN≠PASS.  
R3.2 sole executor. SHADOW default.
