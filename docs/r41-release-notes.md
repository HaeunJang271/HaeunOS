# R4.1 Release Notes

## Scope

Research validation: verified related work, contribution refinement, smoke+standard evidence, paper/presentation revision, reproduction procedure. **No kernel process source changes.**

## Deliverables

- `docs/r41-*.md`
- `research/related-work/*`
- `research/reproduction/*`
- Updated `research/paper/main.md`, presentation + reviewer questions
- `tests/r41_validation_tests.c`

## Verification (2026-11-18)

- Kernel SHA-256 unchanged vs R4.0 baseline.
- `make clean` / `make all` / `make test` exit 0.
- Smoke+standard campaigns PASS (`build/r41-smoke`, `build/r41-standard`).
- Export paths are single-level (`build/r41-smoke`) because `_mkdir` creates one directory level.

## Non-goals

- New scheduling policies
- Fabricated citations/results
- Auto research/100-rep campaigns
- Production autonomy claims
