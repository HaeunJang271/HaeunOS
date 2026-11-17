# R3.2 Failure Injection

## Model

Host-only, test-armed points via `haeun_recfg_fi_arm`. Default `NONE`. Production CLI clears FI before runs. Guest scheduler code is untouched.

## Points

| Point | Effect |
|---|---|
| BEFORE_SNAPSHOT | Reject in `request` |
| AFTER_SNAPSHOT | Reject after validate snapshot |
| AFTER_VALIDATE | Reject after validation |
| BEFORE_PREPARE | Reject entering prepare |
| AFTER_PREPARE | Reject after PREPARED gates |
| BEFORE_COMMIT | Reject before write |
| AFTER_WRITE | FAILED after policy write |
| AFTER_VERIFY | FAILED after successful verify path arm |
| DURING_ROLLBACK | FAILED mid-rollback |
| AFTER_ROLLBACK | FAILED after restore verify |

Each armed point hits once then clears.

## Required coverage

Unit tests exercise every point and assert final txn state plus guest policy outcome (unchanged on reject; restored or FAILED after write paths).
