# R3.2 Transaction State Machine

## States

| State | Meaning |
|---|---|
| IDLE | No active transaction (implied before `request`) |
| REQUESTED | Source/target accepted; active policy matched |
| PREFLIGHT | Entering prepare gates |
| SNAPSHOT_CAPTURED | R3.1 snapshot sealed (host copy) |
| VALIDATING | R3.1 invariants/timestamp/compat running |
| PREPARED | All mandatory gates PASS; policy unchanged |
| COMMITTING | Safe-point policy word write in progress |
| COMMITTED | Target policy verified (terminal success) |
| VERIFYING | Post-write policy read-back |
| ROLLING_BACK | Restoring previous policy ID |
| ROLLED_BACK | Policy ID restored (terminal) |
| REJECTED | Safe reject; guest policy unchanged (terminal) |
| FAILED | Commit/rollback inconsistency; diagnostics kept |

## Allowed transitions

```
IDLE → REQUESTED
REQUESTED → PREFLIGHT | REJECTED
PREFLIGHT → SNAPSHOT_CAPTURED | REJECTED
SNAPSHOT_CAPTURED → VALIDATING | REJECTED
VALIDATING → PREPARED | REJECTED
PREPARED → COMMITTING | REJECTED
COMMITTING → VERIFYING | FAILED | REJECTED
VERIFYING → COMMITTED | FAILED
FAILED → ROLLING_BACK
ROLLING_BACK → ROLLED_BACK | FAILED
```

Invalid edges return `HAEUN_RECFG_BAD_STATE` without mutating guest RAM.

## Invariants

- At most one host transaction owns the protocol (`g_busy`).
- Terminal states are never silently reopened; new work needs a new `txn_id`.
- `PREPARED` must not change `0xA474`.
- `COMMITTED` requires observed policy == target.
- `REJECTED` requires observed policy unchanged from request.
- Rollback scope is **policy ID word only**.

## Failure behavior

- Mandatory R3.1 FAIL/UNKNOWN/blocking WARN → REJECTED.
- Stale snapshot at commit → REJECTED (no write).
- Post-write verify miss → FAILED (rollback eligible).
- Rollback verify miss / FI during rollback → FAILED; further transitions stopped until new request after busy clear on FAILED paths that clear `g_busy`.

## Logging

Each state change appends to the transaction log (state + message). Logging is host-only.
