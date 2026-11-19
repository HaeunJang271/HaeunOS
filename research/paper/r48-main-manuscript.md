# HaeunOS AROS: Host-Assisted Evaluation and Safety-Gated Application of Runtime Scheduling Policies in a Research VM

**Manuscript role:** R4.8 revised research manuscript; R4.8.25 contribution reframing (outside the frozen R4.6 publication package).  
**Does not overwrite:** `research/paper/r45-main-manuscript.md`, `research/paper/main.md`, or any `v4.6.0` package member.  
**Implementation baseline (unchanged):** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Claim policy:** A/B only; C forbidden (`research/paper/r44-publication-claims.md`).  
**Revision basis:** R4.8.1–R4.8.5 claim alignment; R4.8.25 safety-architecture reframing.

Evidence tags: **[V]** verified in freeze sources/tests; **[O]** observed in recorded experiments/logs; **[L]** explicit limitation; **[H]** hypothesis / positioning only.

**Primary contribution (this revision):** Host-mediated safety-gated application of a narrow guest scheduling-policy word in a single-threaded research VM—not scheduling-performance superiority, recommendation diversity, or workload-dependent policy selection.

---

## Abstract

If runtime policy *recommendation* and guest policy *mutation* share the same authority and procedure, the control boundary becomes unclear. We present **HaeunOS AROS**, a host-assisted research pipeline on a modular monolithic HAEUN-32 guest—a custom 32-bit RISC-like ISA (**not** RISC-V)—executed only by a C17 interpreter (`src/vm.c`). Guest instructions are never executed natively by the host. Within HaeunOS, AROS is unrelated to Amiga AROS (AROS Research Operating System); the acronym overlap is coincidental.

AROS separates **evaluation**, **recommendation**, and **mutation**. Mutation of the guest policy word at `0xA474` uses a **host-mediated fail-closed validation path**: SHADOW is the default (evaluate/report without write); APPLY requires explicit opt-in; prepare/validate must pass (UNKNOWN≠PASS) before a distinct commit. Recommendation alone never authorizes the policy word. Prepare/validate/commit operate over guest-visible state from the host—not a “guest-controlled safety boundary.”

**[O]** Primary control evidence (R4.3): bidirectional RR↔FIFO applies reported `COMMITTED`; invalid-target requests are rejected before any policy-word write, during the request-path `known()` check; injected AFTER_WRITE failure restored policy ID only (`scope=policy_id_only`); host-mirror matches of post-apply `curr` were 61/61 per direction in audit/reproduction logs (counts are **not** fields of `r43-active-apply.json`). **[O][L]** Supporting experiments (R4.2 catalog; post-`v4.6.0` R47): descriptive ready-wait favored RR and recommendations collapsed to RR; R47 did not demonstrate real workload-dependent FIFO selection.

**[L]** We do not claim statistical significance, general performance superiority, fairness improvement, workload-dependent discrimination, SMP or hardware atomicity, full-system rollback, unattended APPLY, or third-party reproduction (**NOT TESTED**).

## 1. Introduction

Adaptive and autonomic systems research argues for external control loops and online reconfiguration [S01,S05,S06,S04]. Scheduling work provides flexible mechanisms [S09,S12]; workshop systems explore learned in-kernel policy with verification [S13]. Less often made explicit—in a *research VM*—is that **evaluating** a policy, **recommending** a policy, and **mutating** guest policy state are distinct operations that must not share a single unchecked authority.

**Primary research question.** Can a host-assisted framework evaluate runtime scheduling policies and recommend changes while ensuring that mutating the guest policy word requires a fail-closed validation path distinct from recommendation?

**Secondary questions.** (S1) After recorded APPLY, does a host mirror of selection rules agree with observed `curr` under both target policies within a scoped workload? (S2) Under frozen optimizer profiles and tested workloads, do recommendations diversify across RR and FIFO? (S3) After a failed policy-word write, what state is restored?

We answer **yes for the safety-gated evaluate→recommend→validate→apply composition** on a single-threaded research VM with two policies (RR=0, preemptive FIFO=1) **[V][O]**. We do **not** answer that recommendations diversify or that scheduling performance generalizes **[O][L]**.

## 2. Contributions and non-contributions

1. **Separation of recommendation and mutation [V]:** Host-side evaluation/recommendation does not by itself write the guest policy word; SHADOW default keeps observation non-mutating.
2. **Host-mediated fail-closed application path [V]:** Explicit APPLY opt-in; prepare/validate/commit sequencing; UNKNOWN/FAIL reject before write; prepare ≠ commit.
3. **Explicitly scoped validation and limitations [V][O][L]:** Single-threaded between-`haeun_vm_step()` commit design; recorded bidirectional COMMITTED applies and policy-ID-only rollback; no claim of SMP, hardware atomicity, ACID, or full VM-state rollback.

Supporting observations (R4.2 descriptive catalog metrics; R47 recommendation-collapse follow-up) contextualize the optimizer surface but are **not** reframed as primary contributions **[O][L]**.

**Non-contributions:** statistical significance of RR vs FIFO; fairness improvement; SMP/HW atomicity; full VM rollback; unsupervised always-on APPLY; multi-policy portfolio claims; demonstrated workload-dependent policy discrimination.

## 3. Background and related work

We position this work against autonomic/MAPE visions [S01,S02], self-adaptive surveys [S05], architecture-based external control [S06,S07,S08], OS online reconfiguration and hot-swapping [S03,S04], flexible scheduling [S09,S12], fairness indexing [S10], artifact norms [S11], multicore scheduling principles [S14], and dynamic OS reconfiguration latency [S15]. Learned kernel datapaths with in-kernel verification [S13] contrast with our *host-side* locus of evaluation and mutation gating.

**Distinction [H]:** Compositional positioning of host-mediated evaluate→recommend with UNKNOWN≠PASS apply of a *narrow* guest policy word on a research ISA—not superiority over each cited system. Access grades for S01–S15: `research/related-work/verified-sources.md`.

## 4. Architecture and implementation

**HAEUN-32** is a custom 32-bit RISC-like guest ISA (**not** RISC-V). Guest code runs only in the host C17 interpreter (`src/vm.c`). The guest kernel is **modular monolithic** (not a microkernel claim) with process scheduling and observational telemetry, including `ready_seq` for FIFO ordering **[V]**.

Host research tooling (planner, optimizer, feedback, robustness, policy validation, reconfiguration) runs outside the guest and reads guest RAM snapshots. The guest kernel does **not** independently authorize APPLY. Policy identity resides at guest word `0xA474`. Only RR (`0`) and preemptive FIFO (`1`, older `ready_seq` first) are studied (`kernel/process/proc_policy.s`) **[V]**.

**Workflow [V]:** fixed HXF workloads → guest scheduler/telemetry → host monitor/planner → `src/opt_*.c` scoring → SHADOW (default) or explicit APPLY → `haeun_opt_apply` → `haeun_recfg_*` only.

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

**Transactional policy application** means: prepare/commit sequencing for a single policy-word mutation under single-threaded interpretation; **not** ACID, **not** full VM/PCB/memory rollback, and **not** hardware atomicity **[V][L]**. Short form: *safety-gated application of a narrow guest policy word in a single-threaded research VM* via a **host-mediated fail-closed validation path**.

Prepare ≠ commit **[V]**. Rollback evidence: `research/experiments/r43-rollback-verification.json` (`scope=policy_id_only`) **[O][L]**. Recorded invalid-target rejection blocked the write before mutation, during the request-path `known()` check (`research/experiments/r43-safety-rejection.json`) **[O]**. This does not imply exhaustive invalid-input coverage **[L]**.

## 6. Experimental methodology

| Campaign | Role in this paper | Artifact |
|----------|--------------------|----------|
| R4.3 active apply / audit / rollback | **Primary** control evidence | `r43-active-apply.json`; `docs/r43-active-apply-audit.md`; `r43-rollback-verification.json` |
| R4.2 performance / profiles / stats | **Supporting** scheduling context | `r42-performance-results.json`; `r42-profile-verification.json`; `r42-statistics.json` |
| R47 Gates A–D2 | **Supporting** recommendation-limitation follow-up (post-`v4.6.0`) | `docs/r47-final-evidence-report.md`; `research/experiments/r47-discrimination/` |
| R4.3.2 worktree | Author-host rebuild notes | `docs/r432-reproduction-evidence.md` |

**Enums kept distinct:** `SELECT_RR` / `SELECT_FIFO` versus `MEASURED_*`; `SCORE_EQUIVALENCE` ≠ `MEASURED_EQUIVALENCE`.

**Not tested:** per-profile-cell live APPLY for all 30 R4.2 cells (`NOT_TESTED_per_cell`) **[L]**.

## 7. Results

### 7.1 Primary: safety and control evidence (R4.3)

**Scope:** behavioral APPLY on the recorded apply workload; host mirror versus observed `curr`; documented rejection and fault-injection rollback paths.

**Observations [O]:**
- `r43-active-apply.json`: `RR_to_FIFO_behavioral` and `FIFO_to_RR_behavioral` both `txn=COMMITTED` (`pol_after` 1 then 0).
- Audit/reproduction logs: 61/61 mirror matches per direction (`docs/r43-active-apply-audit.md`; `docs/r432-reproduction-evidence.md`). Match counts are **not** stored in the apply JSON.
- Invalid-target requests are rejected before any policy-word write, during the request-path `known()` check; policy unchanged (`r43-safety-rejection.json`).
- Rollback after FI `AFTER_WRITE`: `policy_id_restored=true`, `scope=policy_id_only`.

**Conclude:** scoped Observed host-mediated apply/reject/rollback for a narrow policy word.  
**Do not conclude:** universal kernel correctness, exhaustive FI coverage, or independent third-party confirmation of the 61/61 counts.

### 7.2 Supporting: R4.2 catalog (descriptive context)

**Scope:** six HXF workloads; RR and FIFO; ten repetitions per workload×policy; five frozen recommendation profiles.

**Observations [O]:**
- `planned_runs=120`, `success_runs=120`, `fail_runs=0`.
- Descriptive mean `ready_wait` lower under RR on all six workloads; Jain means equal within each workload row.
- All 30 profile cells: `recommended_policy=0` (RR); `apply_path=NOT_TESTED_per_cell`.
- `inferential_tests=NOT_PERFORMED`.

**Conclude:** technical catalog context and recommendation collapse to RR.  
**Do not conclude:** inferential superiority, fairness improvement, or a general scheduling-performance contribution.

### 7.3 Supporting: R47 recommendation-discrimination follow-up

**Label:** Post-`v4.6.0` isolated track; **outside** the frozen R4.6 package (`docs/r47-final-evidence-report.md`).

**Gate A (synthetic only):** `tests/r47_opt_discrimination_unit_tests.c` (S1–S9), including synthetic `SELECT_FIFO`. No standalone results JSON; **not** independent reproduction; **not** real-workload discrimination **[L]**.

**Gate B:** `r12_pair`, n=3; decision `SELECT_RR`; measured outcomes **all `NOT_EVALUATED`**—do not merge with C/D1/D2 measured labels **[O][L]**.

**Gates C/D1/D2 (W1–W3):** all completed profile cells `SELECT_RR`; overall `MEASURED_RR_BETTER` on ready; jain/dispatch/completed typically `MEASURED_EQUIVALENCE`; no real `SELECT_FIFO` or `MEASURED_FIFO_BETTER` **[O]**. D1 preserves a 300k step-cap failure (`INSUFFICIENT_TELEMETRY`) distinct from the separately approved 800k confirmatory campaign **[O][L]**.

**W4:** Discontinued (`R4754_W4_NOT_DISTINCT`); not executed **[L]**.

**Conclude:** R47 does **not** demonstrate workload-dependent policy discrimination; it documents recommendation collapse under the tested real cells **[O][L]**.

## 8. Statistical limitations

n=10 deterministic catalog repeats are **not** inferential significance **[L]**. No hypothesis tests for RR vs FIFO superiority (`inferential_tests=NOT_PERFORMED`). Guest ticks ≠ wall-clock.

## 9. Discussion

The primary result is structural: recommendation is not authorization; mutation requires a host-mediated fail-closed path on a narrow policy word **[V][O]**. Supporting scheduling tables show ready-wait magnitudes and RR-only recommendations; they must not be read as general performance wins or as a validated workload-dependent selector **[L]**. Compared with in-kernel learned policies [S13], evaluation and gating remain host-side by design.

## 10. Threats to validity and limitations

**Internal:** host mirror as behavioral proxy; thin rejection corpus; limited fault injection.  
**External:** research ISA; six catalog HXF workloads; two policies; Windows/MinGW author host for freeze evidence.  
**Construct:** automated evaluation ≠ unattended APPLY.  
**Conclusion:** same-host reproduction ≠ independent lab.

**Explicit limitations [L]:** two policies only; RR recommendation collapse; no real FIFO recommendation or `MEASURED_FIFO_BETTER`; descriptive-only performance; no inferential testing; no fairness improvement; single-threaded interpreter; no SMP/hardware atomicity; policy-ID-only rollback; limited FI; author-host reproduction only; third-party **NOT TESTED**; external DNS opt-in (`docs/limitations.md`) not exercised in reported Linux CI.

## 11. Future work (separate from results)

Richer fault-injection matrices, third-party reproduction, and any new telemetry/metrics would require separate freezes. They are not demonstrated here.

## 12. Conclusion

Within a single-threaded research VM and a two-policy surface, HaeunOS AROS demonstrates host-assisted evaluate→recommend with a **host-mediated fail-closed** prepare/commit path for a narrow guest policy word **[V]**. Scoped Observed R4.3 evidence records bidirectional COMMITTED applies, invalid-target rejection before write (request-path `known()` check), and policy-ID-only rollback, with 61/61 mirror agreement in audit/reproduction logs **[O]**.

Supporting R4.2/R47 experiments show RR-oriented descriptive metrics and recommendation collapse; they do **not** establish statistical significance, general performance superiority, workload-dependent discrimination, fairness gains, SMP/atomicity, full-system rollback, unattended APPLY, or third-party reproduction **[L]**.

## 13. References

S01–S15 as catalogued in `research/related-work/verified-sources.md` (access grades M/A/F preserved; see `research/paper/r45-citation-audit.md`). Internal evidence: R4.4 audit `3de73d4…`, implementation freeze `386c3a1…`, R47 consolidation `docs/r47-final-evidence-report.md`.
