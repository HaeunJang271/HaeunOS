# R4.0 Experiment Results

Date: 2026-11-18. Source: executed `make clean` / `make all` / `make test` (exit 0).  
Artifacts: `build/r40/robustness-report.{txt,json}`, `build/r40-make-test.log`.

| Campaign | Preset | Command | Exit | Evidence |
|----------|--------|---------|------|----------|
| Smoke (R4.0 integration) | SMOKE reps=3 | `./build/r40-integration-tests.exe build` | **0** | **OBSERVED** |
| Standard | reps=10 | — | — | **NOT TESTED** |
| Extended | reps=30 | — | — | **NOT TESTED** |
| Research | reps=100 | — | — | **NOT TESTED** |

## Execution log (excerpt)

```
=== R4.0 integration smoke ===
host_tooling_wall_s=0.271 (clock(); research-tool overhead)
overall=0 wl=6
overall=PASS reason=reproducible sufficient evidence under preset
stress transitions=0 failures=0
r40 integration smoke passed
EXIT=0
```

Config: `build_identifier=r40-smoke`, `preset=0`, `reps=3`, `profile_id=0`, `config_hash=0xab1dcbe0` (2870856672).

## Per-workload summary (smoke)

Enum: replay 0=`EXACT_MATCH`, reliability 0=`SUFFICIENT`, sensitivity 0=`STABLE`, verify 0=`CONFIRMED`.

| Workload | Seed | Replay | Rel | Sens | Verify |
|----------|------|--------|-----|------|--------|
| cpu_bound | 0x98543630 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| short_job | 0xd17bbba9 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| mixed | 0xc46199b0 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| bursty | 0xf8edec23 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| block_sleep | 0x52f2dc6f | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| block_wait | 0x54a993f1 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |

Overall: **PASS** — “reproducible sufficient evidence under preset.”  
Stress in this binary path: dry (`haeun_rb_reconfig_stress(..., NULL, 1)`); transitions=0. Live stress remains covered by `r37_robust_guest_tests`.

## Regression suite

| Command | Exit | Class |
|---------|------|-------|
| `make clean` | 0 | VERIFIED |
| `make all` | 0 | VERIFIED |
| `make test` (incl. R1.1–R3.7 + R4.0) | 0 | VERIFIED |

## Limits of this evidence

- Smoke N=3 only; descriptive, not inferential.
- Fixed HXF catalog; no RNG variation.
- Host `clock()` wall seconds are research-tool overhead, not guest latency.
