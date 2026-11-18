# R4.4 Safety & Invariant Audit

**Baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Primary code:** `src/reconfig_txn.c`, `src/pval_*.c`, `src/opt_apply.c`, `include/haeun/reconfig.h`  
**Artifacts:** `research/experiments/r43-safety-rejection.json`, `r43-rollback-verification.json`  
**Tests:** `tests/r32_*`, `r34_*`, `r43_active_verification_tests.c`

## Principle grades

| ID | Principle | Grade | Evidence |
|----|-----------|-------|----------|
| A | SHADOW default | VERIFIED | `opt_cli` uses SHADOW unless APPLY mode; docs `r40-safety-model.md` |
| B | APPLY explicit opt-in | VERIFIED | `HAEUN_OPT_MODE_APPLY` path in `opt_cli.c` / `opt_apply.c` |
| C | UNKNOWN ≠ PASS | VERIFIED | `invariants_ok_for_prepare` rejects UNKNOWN/FAIL (`reconfig_txn.c`) |
| D | Optimizer cannot authorize | VERIFIED | `haeun_opt_apply` only via `haeun_recfg_prepare/commit` |
| E | Kernel/safety layer final auth | VERIFIED | prepare gates on pval snapshot/invariants/compat |
| F | Prepare ≠ Commit | VERIFIED | separate APIs; commit comment “between-step safe point” |
| G | Safe point = between `haeun_vm_step` | PARTIAL | Host API contracts + tests; not a formal proof of all call sites |
| H | Rollback = policy ID only | OBSERVED | `r43-rollback-verification.json` `scope=policy_id_only`; code restores prior ID |
| I | No SMP / no HW atomicity claim | VERIFIED (by non-claim + model) | `r40-safety-model.md`; single-thread interpreter assumption |

## Separated concepts (do not conflate)

| Concept | Status in HaeunOS AROS |
|---------|------------------------|
| Single-thread interpreter serialization | ASSUMED / design basis |
| Hardware atomicity | **NOT CLAIMED** |
| SMP safety | **NOT CLAIMED / NOT TESTED** |
| Policy-ID rollback | **OBSERVED** |
| Full VM / memory / PCB rollback | **NOT CLAIMED** |

## Invariant surface (guest)

| Check | Where | Grade |
|-------|-------|-------|
| Policy ∈ {0,1} | pval + prepare | VERIFIED |
| ready_seq uniqueness / wrap | `pval_timestamp.c` + kernel telem | VERIFIED / OBSERVED |
| PCB / curr consistency | pval invariants | VERIFIED (WARN edges documented) |
| TXN sole writer of policy | design + reconfig path | VERIFIED |

## Active safety observations (R4.3)

| Case | Result | Grade |
|------|--------|-------|
| Invalid policy target | blocked; pol unchanged | OBSERVED |
| FI AFTER_WRITE + rollback | policy ID restored | OBSERVED |
| RR→FIFO / FIFO→RR commit | COMMITTED + mirror 61/61 | OBSERVED |

## Limitations

- Safe-point guarantee is **software convention** on the host interpreter, not CPU atomicity.
- Rejection corpus in JSON is thin (`invalid_policy` primary); richer adversarial suite remains desirable.
- Guest kernel does not “approve” host writes; trust boundary is host tooling correctness + pval gates.
