# R4.6 Final Release Audit

**Release status: FINAL_FREEZE_READY**  
**HEAD (pre-release commit):** `3de73d4ee0649d990bc0da5904872f24767fe53e`  
**Mode:** documentation freeze preparation only (no staging/commit/push in this step)

## Immutable references (verified)

| Role | SHA |
|------|-----|
| Implementation baseline | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| R4.3.2 evidence docs | `735013eccec1f48f814245082238b94c04f4e288` |
| R4.4 research audit | `3de73d4ee0649d990bc0da5904872f24767fe53e` (= current HEAD) |

## Phase 1 — Inventory

**18 publication/audit package files** enumerated (see `docs/r46-release-manifest.json`).  
F1 corrected edges verified:

- `OPT -->|SHADOW default| REP` (`f1-system-architecture.md` L27)
- `OPT -->|APPLY opt-in| SAF` (L28)
- `SAF -->|prepare then| TXN` (L29)
- No SHADOW→SAF edge

## Phase 2 — Integrity checklist

| # | Check | Result |
|---|-------|--------|
| 1 | Claims traceable | PASS (R4.5.1 matrix + freeze sources) |
| 2 | Numbers match artifacts | PASS (120/6/2/n=10/Jain/ready_wait/30/61/61 sources) |
| 3 | Third-party NOT TESTED | PASS |
| 4 | No statistical significance claim | PASS |
| 5 | Jain ≠ fairness improvement | PASS |
| 6 | 30-cell live APPLY NOT_TESTED_per_cell | PASS |
| 7 | Rollback policy-ID-only | PASS |
| 8 | No SMP / HW atomicity claims | PASS |
| 9 | No unattended automatic APPLY | PASS |
| 10 | Figures match manuscript semantics | PASS (F1 aligned with F2/§5 after R4.5.2) |

## Phase 3 — Reproducibility package

`research/reproduction/r45/` documents freeze checkout `386c3a1…`, author env, `make clean && make all && make test`, artifact index, limitations.  
**Does not** claim third-party success. No experiments executed in R4.6.

## Phase 4 — Release docs (this milestone)

- `docs/r46-final-release-audit.md` (this file)
- `docs/r46-release-manifest.json`
- `docs/r46-release-notes.md`

## Remaining limitations (carry forward)

1. Third-party reproduction NOT TESTED  
2. Individual make exit codes NOT CAPTURED (chain exit 0 only)  
3. Two policies; 30/30 RR recommendations; per-cell live APPLY not tested  
4. Descriptive catalog metrics only; deterministic HXF  
5. Policy-ID rollback only; single-thread safe-point convention  

## Git integrity (this step)

- Staged: none  
- Implementation/experiments dirty: none  
- EXCLUDE 12: retained untracked  
- No commit/push performed  

**Await explicit approval** before staging the publication package (+ optional R4.6 docs) and creating a release commit/tag.
