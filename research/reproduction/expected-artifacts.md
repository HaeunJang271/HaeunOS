# Expected Artifacts (R4.1)

| Path | Description |
|------|-------------|
| `build/haeun-vm.exe` (or `build/haeun-vm`) | Host VM |
| `build/r41-validation-tests.exe` | Campaign runner |
| `build/r41-smoke/robustness-report.json` | Smoke machine-readable |
| `build/r41-smoke/robustness-report.txt` | Smoke human-readable |
| `build/r41-standard/robustness-report.json` | Standard machine-readable |
| `build/r41-standard/robustness-report.txt` | Standard human-readable |

## Expected smoke properties (if successful)

- `overall`: PASS (or non-FAIL)
- `reps`: 3
- 6 workloads present
- Prefer `replay` EXACT_MATCH / METRIC_MATCH

## Expected standard properties

- `reps`: 10
- baseline matrix conceptually 120 candidate runs
- Same workload IDs

Exact numeric guest metrics may match across identical builds; host wall times will differ.
