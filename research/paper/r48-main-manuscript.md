# HaeunOS AROS: Host-Assisted Evaluation and Safety-Gated Application of Runtime Scheduling Policies in a Research VM

**Manuscript role:** R4.8 revised research manuscript; R4.8.3 implementation revision (outside the frozen R4.6 publication package).  
**Does not overwrite:** `research/paper/r45-main-manuscript.md`, `research/paper/main.md`, or any `v4.6.0` package member.  
**Implementation baseline (unchanged):** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Claim policy:** A/B only; C forbidden (`research/paper/r44-publication-claims.md`).  
**Revision basis:** R4.8.1 claim-to-evidence audit; R4.8.2 revision design.

Evidence tags: **[V]** verified in freeze sources/tests; **[O]** observed in recorded experiments/logs; **[L]** explicit limitation; **[H]** hypothesis / positioning only.

**Positioning:** Host-assisted runtime policy evaluation and recommendation, with safety-gated application of a narrow guest policy word in a single-threaded research VM.

---

## Abstract

Operating-system scheduling policy is often fixed early, leaving a gap between host-side measurement and safe mutation of guest policy state. We present **HaeunOS AROS**, a research label for a host-assisted pipeline on a modular monolithic HAEUN-32 guest—a custom 32-bit RISC-like ISA (not RISC-V)—executed by a C17 interpreter (`src/vm.c`). Guest instructions are never executed natively by the host. Within HaeunOS, AROS is unrelated to Amiga AROS (AROS Research Operating System); the acronym overlap is coincidental.

The pipeline separates **evaluation**, **recommendation**, **fail-closed safety validation**, and **opt-in application** of the guest policy word at `0xA474`. Only two policies are studied: round-robin (RR=0) and preemptive FIFO (policy=1). SHADOW is the default (evaluate/report without mutation); APPLY requires explicit opt-in. A host recommendation never alone authorizes the policy word: mutation requires prepare (UNKNOWN≠PASS) and a distinct commit. Prepare/validate/commit are **host-mediated** over guest-visible state—not a “guest-controlled safety boundary.”

**[O]** In recorded R4.3 behavioral runs, RR↔FIFO applies reported `COMMITTED`, and a host mirror of guest selection matched observed post-apply `curr` on 61/61 samples per direction in audit/reproduction logs. Match counts are **not** fields of `research/experiments/r43-active-apply.json` (which records COMMITTED transitions). This is scoped observation, not general correctness. **[O]** On a six-workload HXF catalog (120 successful R4.2 runs), descriptive mean guest `ready_wait` was lower under RR; Jain means were equal per workload; all 30 R4.2 profile cells recommended RR. **[O][L]** A post-`v4.6.0` R47 discrimination follow-up (outside the frozen R4.6 package) again observed only `SELECT_RR` on completed real W1–W3 cells across five profiles, with `avg_ready_wait` as the sole differentiating optimizer input and no real `SELECT_FIFO` or `MEASURED_FIFO_BETTER`.

**[L]** We do not claim statistical significance, general performance superiority, workload-dependent policy discrimination, fairness improvement, SMP or hardware atomicity, full-system rollback, unattended APPLY, or third-party reproduction (author-host rebuild only; third-party **NOT TESTED**).

## 1. Introduction

Adaptive and autonomic systems research has long argued for external control loops and online reconfiguration [S01,S05,S06,S04]. Scheduling research provides flexible policy mechanisms [S09,S12], while workshop work explores learned in-kernel policy with verification [S13]. Less often made explicit is a *research-VM* composition in which host tooling evaluates telemetry and may recommend a scheduling policy, while mutating the guest policy word requires a fail-closed validation path distinct from recommendation.

**Primary research question.** Can a host-assisted framework evaluate runtime scheduling policies and recommend changes while ensuring mutation of the guest policy word requires a fail-closed validation path distinct from recommendation?

**Secondary questions.** (S1) After recorded APPLY, does a host mirror of selection rules agree with observed `curr` under both target policies within a scoped workload? (S2) Under frozen optimizer profiles and tested workloads, do recommendations diversify across RR and FIFO? (S3) After a failed policy-word write, what state is restored?

We answer **yes for the safety-gated evaluate→recommend→validate→apply composition** on a single-threaded research VM with two policies **[V][O]**. We do **not** answer that recommendations diversify or that performance generalizes **[O][L]**.

## 2. Contributions and non-contributions

1. **Implemented architecture [V]:** Host-side evaluate→recommend pipeline with SHADOW default and APPLY opt-in.
2. **Host-mediated fail-closed safety path [V]:** Recommendation ≠ policy-word authorization; prepare rejects UNKNOWN/FAIL; prepare ≠ commit.
3. **Scoped observed application behavior [O]:** Bidirectional COMMITTED apply and 61/61 mirror matches in recorded R4.3 evidence.
4. **Descriptive experiment observations [O]:** R4.2 catalog metrics and 30/30 RR recommendations; R47 follow-up recommendation collapse.
5. **Reproducibility documentation [O][L]:** Author-host freeze rebuild chain exit 0; third-party NOT TESTED.

**Empirical limitation (not a success claim) [O][L]:** Recommendation collapse to RR under catalog and R47 real cells; no real FIFO recommendation or measured FIFO superiority.

**Non-contributions:** statistical significance of RR vs FIFO; fairness improvement; SMP/HW atomicity; full VM rollback; unsupervised always-on APPLY; multi-policy portfolio claims; workload-dependent policy discrimination.

## 3. Background and related work

We position this work against autonomic/MAPE visions [S01,S02], self-adaptive surveys [S05], architecture-based external control [S06,S07,S08], OS online reconfiguration and hot-swapping [S03,S04], flexible scheduling [S09,S12], fairness indexing [S10], artifact norms [S11], multicore scheduling principles [S14], and dynamic OS reconfiguration latency [S15]. Learned kernel datapaths with in-kernel verification [S13] contrast with our *host-side* locus of evaluation.

**Distinction [H]:** Compositional positioning of host-mediated evaluate→recommend with UNKNOWN≠PASS apply of a *narrow* guest policy word on a research ISA—not a claim of superiority over each cited system. Access grades for S01–S15 are preserved in `research/related-work/verified-sources.md`.

## 4. Architecture and implementation

**HAEUN-32** is a custom 32-bit RISC-like guest ISA (not RISC-V). Guest code runs only in the host C17 interpreter (`src/vm.c`); the host never executes HAEUN instructions natively. The guest kernel is **modular monolithic** (not a microkernel claim) with process scheduling and observational telemetry, including `ready_seq` for FIFO ordering **[V]**.

Host research tooling (planner, optimizer, feedback, robustness, policy validation, reconfiguration) runs outside the guest and reads guest RAM snapshots. Policy identity resides at guest word `0xA474`. Only RR (`0`) and preemptive FIFO (`1`, older `ready_seq` first) are studied (`kernel/process/proc_policy.s`) **[V]**.

**Workflow [V]:** fixed HXF workloads → guest scheduler/telemetry (`proc_sched.s`, `proc_policy.s`, `proc_telem.s`) → host monitor/planner → `src/opt_*.c` scoring → SHADOW (default) or explicit APPLY → `haeun_opt_apply` → `haeun_recfg_*` only.

**Safe point [V/L]:** Commits are designed for a between-`haeun_vm_step()` boundary under single-threaded interpretation. This does **not** establish SMP safety or hardware atomicity **[L]**.

## 5. Safety model and transactional policy application

Documented invariants include recognized policy IDs {0,1}, PCB/curr consistency checks, and ready_seq constraints for FIFO ordering evidence **[V]**.

| Term | Meaning |
|------|---------|
| Validation | Guest-state checks via `haeun_pval_*` during prepare |
| Prepare | Snapshot + validate; **no** policy write; UNKNOWN/FAIL reject |
| Commit | Write target policy ID at `0xA474` |
| Policy application | Opt-in APPLY path ending in commit |
| Failure handling | Reject before write, or FAILED after write → rollback |
| Policy-ID-only rollback | Restore `source_policy` only (`haeun_recfg_rollback`) |

**Transactional policy application** means: prepare/commit sequencing for a single policy-word mutation under single-threaded interpretation; **not** ACID, **not** full VM/PCB/memory rollback, and **not** hardware atomicity **[V][L]**.

Prepare ≠ commit **[V]**. Rollback evidence: `research/experiments/r43-rollback-verification.json` (`scope=policy_id_only`) **[O][L]**. In the recorded invalid-target rejection case, the operation was blocked and the policy remained unchanged (`research/experiments/r43-safety-rejection.json`) **[O]**. This does not imply exhaustive invalid-input coverage **[L]**.

## 6. Experimental methodology

| Campaign | Design | Artifact |
|----------|--------|----------|
| R4.2 performance | 6×2×10=120 | `research/experiments/r42-performance-results.json` |
| R4.2 profiles | 6×5=30 cells | `research/experiments/r42-profile-verification.json` |
| R4.2 statistics | descriptive; no inferential | `research/experiments/r42-statistics.json` |
| R4.3 active apply | RR→FIFO, FIFO→RR | `research/experiments/r43-active-apply.json` |
| R4.3 apply audit | mirror match table | `docs/r43-active-apply-audit.md` |
| R4.3 rollback | FI AFTER_WRITE | `research/experiments/r43-rollback-verification.json` |
| R4.3.2 worktree | author-host rebuild | `docs/r432-reproduction-evidence.md` |
| R47 follow-up | Gates A–D2; W1–W3 | `docs/r47-final-evidence-report.md`; `research/experiments/r47-discrimination/` |

**Enums kept distinct:** `SELECT_RR` / `SELECT_FIFO` (optimizer decisions) versus `MEASURED_RR_BETTER` / `MEASURED_FIFO_BETTER` / `MEASURED_EQUIVALENCE` (measured labels); `SCORE_EQUIVALENCE` ≠ `MEASURED_EQUIVALENCE`; `INSUFFICIENT_TELEMETRY` / `NO_DECISION` are separate paths.

**Not tested:** per-profile-cell live APPLY for all 30 cells (`NOT_TESTED_per_cell`) **[L]**.

## 7. Results

### 7.1 R4.2 baseline (catalog)

**Scope:** six HXF workloads; policies RR and FIFO; ten repetitions per workload×policy as recorded; five frozen profiles for recommendation cells.

**Observations [O]:**
- `planned_runs=120`, `success_runs=120`, `fail_runs=0`.
- Descriptive mean `ready_wait` lower under RR on all six workloads (e.g., cpu_bound 749 vs 812; mixed 15439 vs 18778).
- Jain means equal RR vs FIFO within each workload row.
- All 30 profile cells: `recommended_policy=0` (RR); `apply_path=NOT_TESTED_per_cell`.
- `inferential_tests=NOT_PERFORMED`; ready stddev reported as 0 in `r42-statistics.json` for catalog cells.

**Conclude:** descriptive catalog pattern and recommendation collapse to RR.  
**Do not conclude:** inferential superiority, fairness improvement, or per-cell APPLY behavior.

### 7.2 R4.3 bidirectional application

**Scope:** behavioral apply on the recorded apply workload; host mirror versus observed `curr`.

**Observations [O]:**
- `r43-active-apply.json`: `RR_to_FIFO_behavioral` and `FIFO_to_RR_behavioral` both `txn=COMMITTED` (`pol_after` 1 then 0).
- Audit/reproduction logs: 61/61 mirror matches per direction (`docs/r43-active-apply-audit.md`; `docs/r432-reproduction-evidence.md`). The apply JSON does **not** store match counts.
- Rollback after FI `AFTER_WRITE`: `policy_id_restored=true`, `scope=policy_id_only`.

**Conclude:** scoped Observed apply and policy-ID-only rollback on the documented failure path.  
**Do not conclude:** universal kernel/optimizer correctness or independent third-party confirmation of the 61/61 counts.

### 7.3 R47 recommendation-discrimination follow-up (post-publication)

**Label:** Post-`v4.6.0` isolated track; **outside** the frozen R4.6 package; does not rewrite R4.2/R4.3 artifacts (`docs/r47-final-evidence-report.md`).

**Gate A (synthetic only):** `tests/r47_opt_discrimination_unit_tests.c` covers S1–S9 decision branches, including synthetic `SELECT_FIFO`. No standalone Gate A results JSON. Synthetic cases are **not** real-workload discrimination **[L]**.

**Gate B (integration):** `r12_pair`, n=3, balanced; decision `SELECT_RR`; measured outcomes **all `NOT_EVALUATED`**. Do not merge with C/D1/D2 measured labels **[O][L]**.

**Gate C (W1):** 50 successful runs per policy across five profiles (10 repetitions per profile); all five profile cells `SELECT_RR`; overall `MEASURED_RR_BETTER`; ready differs (RR 65771.666667 vs FIFO 78741.0); jain/dispatch/completed `MEASURED_EQUIVALENCE` **[O]**.

**Gate D1 (W2):** Preserved 300k step-cap failure (`INSUFFICIENT_TELEMETRY`); approved 800k confirmatory campaign with 50 successful runs per policy across five profiles (10 repetitions per profile); all five profile cells `SELECT_RR` / overall `MEASURED_RR_BETTER` (ready RR 367021.666667 vs FIFO 461741.0) **[O]**.

**Gate D2 (W3):** 50 successful runs per policy across five profiles (10 repetitions per profile); all five profile cells `SELECT_RR` / `MEASURED_RR_BETTER` (ready RR 76037.666667 vs FIFO 90984.333333). Analysis-only: `total_sleep` 358/417; `total_blocked` 0—not optimizer inputs **[O][L]**.

**Across C/D1/D2:** `avg_ready_wait` was the sole differentiating optimizer input; no real `SELECT_FIFO` or `MEASURED_FIFO_BETTER`. Spec H5 (positive real FIFO discrimination) **not met** **[O][L]**.

**W4:** Discontinued after R4.7.54 (`R4754_W4_NOT_DISTINCT`); not implemented **[L]**.

## 8. Statistical limitations

n=10 deterministic catalog repeats are **not** inferential significance **[L]**. No hypothesis tests for RR vs FIFO superiority (`inferential_tests=NOT_PERFORMED`). Guest ticks ≠ wall-clock. Sample independence is only partial (fresh VM claimed; fixed images). Effect sizes are not primary claims.

## 9. Discussion

Authority separation and fail-closed prepare can be engineered as first-class interfaces between host optimization and guest policy state **[V][O]**. Descriptive ready_wait magnitudes differ by workload, but recorded recommendations collapsed to RR; these tables must not be read as workload-dependent policy choice or general scheduling wins **[L]**. Compared with in-kernel learned policies [S13], the locus of evaluation remains host-side by design.

## 10. Threats to validity and limitations

**Internal:** host mirror as behavioral proxy; thin rejection corpus; limited fault injection.  
**External:** research ISA; six catalog HXF workloads; two policies; Windows/MinGW author host for freeze evidence.  
**Construct:** automated evaluation ≠ unattended APPLY.  
**Conclusion:** same-host reproduction ≠ independent lab.

**Explicit limitations [L]:** two policies only; RR recommendation collapse; no real FIFO recommendation or `MEASURED_FIFO_BETTER`; descriptive-only performance; deterministic repeats without inferential testing; no fairness improvement; single-threaded interpreter; no SMP/hardware atomicity; policy-ID-only rollback; limited FI; author-host reproduction only; third-party **NOT TESTED**; external DNS is opt-in (`docs/limitations.md`) and **not** exercised in reported Linux CI (`make clean && make all && make test` only in `.github/workflows/linux-ci.yml`).

## 11. Future work (separate from results)

Richer fault-injection matrices, third-party reproduction, and any new telemetry/metrics would require separate freezes. They are not demonstrated here.

## 12. Conclusion

Within a single-threaded research VM and a two-policy surface, HaeunOS AROS demonstrates host-assisted evaluate→recommend with a host-mediated, safety-gated prepare/commit path for a narrow guest policy word **[V]**.

Scoped Observed bidirectional apply recorded 61/61 host-mirror agreement in audit/reproduction logs **[O]**.

Recorded recommendations collapsed to RR on the R4.2 catalog and on R47 real W1–W3 cells; stronger claims—statistical significance, general performance superiority, workload-dependent policy discrimination, fairness gains, SMP/atomicity, full-system rollback, unattended APPLY, third-party reproduction—are unsupported and withheld **[L]**.

## 13. References

S01–S15 as catalogued in `research/related-work/verified-sources.md` (access grades M/A/F preserved; see `research/paper/r45-citation-audit.md`). Internal evidence: R4.4 audit `3de73d4…`, implementation freeze `386c3a1…`, R47 consolidation `docs/r47-final-evidence-report.md`.
