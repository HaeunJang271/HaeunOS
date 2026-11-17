# R3.2 Rollback

## Scope (honest)

Rollback restores **only** the policy ID word at `0xA474` to the transaction source policy.

It does **not** claim:

- Full scheduler state restore
- Full VM / PCB / timer restore
- Multi-word transactional undo

Rationale: successful commit mutates only that one word. Restoring it returns the authorized mutation.

## Protocol

1. Allowed from `FAILED` (and related post-write failure paths).
2. Enter `ROLLING_BACK`.
3. Write `source_policy` to `0xA474`.
4. Verify read-back.
5. Set `policy_id_restored`, enter `ROLLED_BACK`.

## Rollback failure

If write/verify fails or FI fires during rollback:

- Enter `FAILED`.
- Do not claim restoration.
- Preserve log / diagnostics.
- Stop overlapping transitions (`g_busy` cleared on this FAILED path so an explicit recovery procedure can start a **new** transaction after operator review).

## Distinctions

| Claim | Implemented? |
|---|---|
| Policy ID restored | Yes, when `ROLLED_BACK` |
| Scheduler state restored | No |
| Entire VM restored | No |
