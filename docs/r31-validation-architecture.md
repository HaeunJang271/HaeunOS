# R3.1 Validation Architecture

Host-only pipeline:

```text
haeun_pval_snapshot_read (const VM → copied snapshot + seal)
        → invariants / timestamp / compatibility
        → haeun_pval_snapshot_verify_unchanged (seal + CSRs)
        → report (text/JSON)
```

No guest steps during validation. No writes to guest RAM or `0xA474`.
Not a second scheduler. Compatibility reports `transition_preconditions_satisfied`, not `transition_safe`.
