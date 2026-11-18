# R4.0 Release Notes

## Scope

Research integration & publication readiness. No new scheduling policies. Prefer no kernel changes.

## Deliverables

- Unified architecture / safety / state-machine docs
- Claim–evidence matrix
- Smoke campaign via `r40-integration-tests`
- Paper draft `research/paper/main.md`
- Presentation outline `research/presentation/outline.md`
- Figures under `research/figures/`

## Kernel integrity

R4.0 prefers host-side integration only. Kernel process sources should match R3.7 SHA-256 baselines recorded in `docs/r40-research-audit.md`.

## Commands

```sh
make clean
make all
make test
# smoke is included via ./build/r40-integration-tests build
```

## Non-goals

- Fabricated benchmarks or citations
- Claiming production autonomy
- Inferential statistical superiority
- Auto-running research/100-rep campaigns
