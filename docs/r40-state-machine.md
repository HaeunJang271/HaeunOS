# R4.0 Reconfiguration State Machine

Actual enum (`haeun_recfg_state`):

```mermaid
stateDiagram-v2
  [*] --> IDLE
  IDLE --> REQUESTED: request
  REQUESTED --> PREFLIGHT: prepare
  REQUESTED --> REJECTED: mismatch/FI
  PREFLIGHT --> SNAPSHOT
  SNAPSHOT --> VALIDATING
  VALIDATING --> PREPARED: gates PASS
  VALIDATING --> REJECTED: FAIL/UNKNOWN/block WARN
  PREPARED --> COMMITTING: commit
  COMMITTING --> VERIFYING: write 0xA474
  COMMITTING --> FAILED: FI after write
  VERIFYING --> COMMITTED: match target
  VERIFYING --> FAILED: mismatch
  FAILED --> ROLLING_BACK: rollback
  ROLLING_BACK --> ROLLED_BACK: policy ID restored
  ROLLING_BACK --> FAILED: restore verify fail
```

## Safe point

Between `haeun_vm_step` calls in a single-threaded interpreter (VERIFIED by R3.2/R3.4 design + tests).

## Active transitions

R3.4 requires eligible≥2 + ready_seq PASS for demonstrated mid-run RR↔FIFO under tested images.
