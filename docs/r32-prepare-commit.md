# R3.2 Prepare and Commit

## Prepare (no policy mutation)

1. Require `REQUESTED`.
2. Run R3.1 `haeun_pval_validate(vm, target)`.
3. Seal copied snapshot fields (policy, curr, timer_count, seal words).
4. Reject if read-only audit ≠ PASS.
5. Reject if invariants are FAIL, UNKNOWN, or blocking WARN.
6. Reject if `transition_preconditions_satisfied` is false (UNKNOWN≠PASS).
7. Reject if snapshot policy ≠ declared source.
8. Enter `PREPARED`. Guest `0xA474` unchanged.

Non-blocking WARN allowlist (documented + tested): `TELEM.stale_free`, `CUR.running_match`.

## Commit (minimal write)

1. Require `PREPARED`.
2. Recheck snapshot unchanged (`haeun_pval_snapshot_verify_unchanged`).
3. Recheck active policy == source.
4. Write target policy ID to `0xA474` only.
5. Verify read-back == target → `COMMITTED`.

No other guest fields are intentionally mutated.

## Direction asymmetry

| Direction | Timestamp gate |
|---|---|
| FIFO → RR | Not required (N/A → may PASS) |
| RR → FIFO | Required; UNKNOWN rejects; vacuous empty READY/RUNNING may PASS |

No PCB sequence epoch was added (would change layout/ABI). Lifetime `<2^31` remains unproven by snapshot → many RR→FIFO cases stay rejected.

## Atomicity wording

Commit is a **serialized single-word store** observed between steps. Do not describe it as ISA-atomic multi-word commit or system-wide atomicity.
