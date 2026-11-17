# R3.1 Validation API

| Function | Role |
|---|---|
| `haeun_pval_snapshot_read` | Const copy + seal |
| `haeun_pval_snapshot_verify_unchanged` | Before/after seal+CSR compare |
| `haeun_pval_invariants_check` | PCB/curr/policy/telem |
| `haeun_pval_timestamp_check` | FIFO modular assumptions |
| `haeun_pval_compat_check` | Source/target preconditions |
| `haeun_pval_validate` | Orchestrator |
| `haeun_pval_report_fprint[_json]` | Reports |

## CLI (tested)

```text
build/haeun-vm.exe build/r12_yield.hxf --validate-policy --target rr
build/haeun-vm.exe build/fcfs/r12_yield.hxf --validate-policy-json --target fcfs
```

Runs the existing guest to halt, then validates without further guest instructions.

## Overall aggregation

FAIL > UNKNOWN > WARN > PASS; NOT_APPLICABLE ignored.
Overall PASS is refused when any required check is FAIL or UNKNOWN.
