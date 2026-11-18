# R4.3 Active Apply Audit

Fresh run exit 0 (`build/r43-active.log`).

## Bidirectional apply

| Transition | Eligible≥2 | Seq OK | Txn | pol_after | Post-apply mirror matches |
|------------|------------|--------|-----|-----------|---------------------------|
| RR→FIFO | yes | yes | COMMITTED | 1 | 61/61 |
| FIFO→RR | yes | yes | COMMITTED | 0 | 61/61 |

Prepare ≠ Commit. Commit ≠ behavioral proof (mirror required).

## Safety rejection / rollback

- Invalid target / prepare gate: policy unchanged (**OBSERVED**).
- FI `AFTER_WRITE` armed+hit; rollback path; scope = **policy ID only** (**OBSERVED**).

## Profile→apply mapping

R4.2: 30/30 recommended RR (**REPORTED** prior).  
R4.3: grouped apply evidence (RR↔FIFO behavioral); `live_apply_per_cell=false` for all 30 cells.

Artifacts: `r43-active-apply.json`, `r43-safety-rejection.json`, `r43-rollback-verification.json`, `r43-profile-apply-mapping.json`.
