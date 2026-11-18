# R4.1 Experiment Results

Date: 2026-11-18. Environment: project development host (Windows).  
Commands: `make clean` (0), `make all` (0), `make test` (0 after export-path fix),  
`./build/r41-validation-tests.exe build both` (0).

| Campaign | Preset | Baseline runs | Host wall (`clock`) | config_hash | overall | Class |
|----------|--------|---------------|---------------------|-------------|---------|-------|
| Smoke | reps=3 | 36 | 0.267 s | `0xab1dcbe0` | PASS | **OBSERVED** |
| Standard | reps=10 | 120 | 0.965 s | `0x4a32cc07` | PASS | **OBSERVED** |
| Extended | 30 | — | — | — | — | **NOT TESTED** |
| Research | 100 | — | — | — | — | **NOT TESTED** |

Artifacts: `build/r41-smoke/`, `build/r41-standard/`, `build/r41-campaign.log`.

## Per-workload (smoke and standard identical classifiers)

Enum: replay0=EXACT_MATCH, rel0=SUFFICIENT, sens0=STABLE, verify0=CONFIRMED.

| Workload | Seed | Replay | Rel | Sens | Verify |
|----------|------|--------|-----|------|--------|
| cpu_bound | 0x98543630 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| short_job | 0xd17bbba9 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| mixed | 0xc46199b0 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| bursty | 0xf8edec23 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| block_sleep | 0x52f2dc6f | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |
| block_wait | 0x54a993f1 | EXACT_MATCH | SUFFICIENT | STABLE | CONFIRMED |

Eval traces: all policy×workload cells `ok=reps fail=0` for smoke and standard.

## Resource decision

Smoke host wall ≪ 1 s → **RUN_STANDARD** documented in runner stdout. Extended/research not authorized.

## Note on wall times

Host `clock()` measures research-tool phase around `haeun_rb_run_experiments`; guest instruction work is separate. Single-sample exploratory; not a kernel overhead claim.
