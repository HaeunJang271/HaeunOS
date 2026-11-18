# HaeunOS AROS: Host-Assisted Evaluation, Recommendation, and Safety-Gated Application of Runtime Scheduling Policies

**Manuscript role:** R4.5 publication package (does not overwrite `research/paper/main.md`).  
**Implementation baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Reproduction evidence tip:** `735013eccec1f48f814245082238b94c04f4e288`  
**Research audit tip:** `3de73d4ee0649d990bc0da5904872f24767fe53e`  
**Claim policy:** follows `research/paper/r44-publication-claims.md` (A/B only; C forbidden).

Evidence tags used below: **[V]** verified in freeze sources/tests; **[O]** observed in recorded experiments/logs; **[L]** explicit limitation; **[H]** hypothesis / positioning only.

---

## Abstract

Operating-system scheduling policy is often chosen early and changed rarely, leaving a gap between host-side measurement and *safe* runtime mutation of guest policy state. We present **HaeunOS AROS**, a research prototype on a modular HAEUN-32 guest that separates **evaluation**, **recommendation**, **safety validation**, and **transactional application** of scheduling policies. Only two guest policies are studied: round-robin (RR=0) and preemptive FIFO (policy=1). A host-assisted optimizer may recommend a policy, but cannot authorize the guest policy word; authorization requires a fail-closed prepare step (UNKNOWN≠PASS) and a distinct commit. **[V]** In recorded R4.3 runs, bidirectional RR↔FIFO applies committed and a host mirror of guest selection matched observed post-apply selections (61/61 each direction). **[O]** On a fixed six-workload HXF catalog (120 successful R4.2 runs), descriptive mean guest `ready_wait` favored RR; these results are **not** inferential proof of superiority. **[L]** We do not claim SMP safety, hardware atomicity, full-system rollback, unattended automatic APPLY, fairness improvement, third-party reproduction, or broad policy-portfolio generalization. Author-host detached worktree rebuild of the freeze SHA completed a chained `make clean && make all && make test` with exit 0; third-party reproduction remains **NOT TESTED**.

## 1. Introduction

Adaptive and autonomic systems research has long argued for external control loops and online reconfiguration [S01,S05,S06,S04]. Scheduling research provides flexible policy mechanisms [S09,S12], while recent workshop work explores learned in-kernel policy with verification [S13]. Less often made explicit is a *research-VM* composition in which host tooling evaluates telemetry, recommends a scheduling policy, and may apply it only through a transactional gate that treats UNKNOWN validation outcomes as non-PASS.

**Research question.** Can a host-assisted autonomous optimization framework safely evaluate, recommend, validate, and apply runtime scheduling policies while preserving explicitly defined kernel invariants?

We answer **yes within qualified evidence** on a single-threaded guest/host research VM with two policies **[V][O]**, and we refuse stronger performance or production claims **[L]**.

## 2. Research question and contributions

**Contributions (publication-safe):**

1. **Architecture [V]:** A host-assisted AROS pipeline with explicit SHADOW default and opt-in APPLY.
2. **Authority separation [V]:** Optimizer recommendation ≠ kernel authorization; apply uses `haeun_recfg_prepare` / `haeun_recfg_commit`.
3. **Safety contract [V]:** Prepare rejects UNKNOWN/FAIL invariant outcomes; commit is separate.
4. **Behavioral apply evidence [O]:** RR↔FIFO commits with host-mirror selection agreement 61/61 in recorded R4.3 executions.
5. **Honest evaluation boundary [L]:** Descriptive catalog metrics only; same-host reproduction; third-party NOT TESTED.

**Non-contributions:** statistical significance of RR vs FIFO; fairness improvement; SMP/HW atomicity; full VM rollback; unsupervised always-on APPLY; multi-policy portfolio claims.

## 3. Background and related work

We position AROS against autonomic/MAPE visions [S01,S02], self-adaptive surveys [S05], architecture-based external control [S06,S07,S08], OS online reconfiguration and hot-swapping [S03,S04], flexible scheduling [S09,S12], fairness indexing [S10], artifact norms [S11], multicore scheduling principles [S14], and dynamic OS reconfiguration latency [S15]. Learned kernel datapaths with in-kernel verification [S13] are a contrasting *in-guest* locus of policy learning.

**Distinction (qualified, not automatic novelty) [H]:** HaeunOS emphasizes *host-mediated* evaluate→recommend with UNKNOWN≠PASS transactional apply of a *narrow* guest policy word on a research ISA—compositional positioning supported by S01–S15 metadata/abstract/full-text statuses in `research/related-work/verified-sources.md`, not a claim of superiority over each system.

## 4. HaeunOS architecture

The guest is a modular monolithic HAEUN-32 kernel with process scheduling and observational telemetry (PCB fields; ready_seq for FIFO ordering). **[V]** The host research tooling (planner, optimizer, feedback, robustness, policy validation, reconfiguration) runs outside the guest and reads guest RAM snapshots. Policy identity resides at guest word `0xA474`. See Figure F1.

## 5. Host-assisted optimization workflow

1. **Workload:** fixed HXF images built from tracked `userland/` + `tools/build_r*.py` **[V]**.  
2. **Guest scheduler / telemetry:** `proc_sched.s`, `proc_policy.s`, `proc_telem.s` **[V]**.  
3. **Host monitor / analysis / planning:** telem inspect, planner modules **[V]**.  
4. **Optimization / recommendation:** `src/opt_*.c` scoring under profiles **[V]**.  
5. **SHADOW vs APPLY:** SHADOW evaluates without mutating policy; APPLY is explicit CLI/mode opt-in **[V]**.  
6. **Safety + transaction:** `haeun_opt_apply` → `haeun_recfg_*` only **[V]**.  

See Figure F2.

## 6. Safety model and invariants

Documented invariants include recognized policy IDs {0,1}, PCB/curr consistency checks, and ready_seq uniqueness/wrap constraints for FIFO ordering evidence **[V]**. Prepare fails closed when mandatory outcomes are UNKNOWN or FAIL **[V]**. Assumptions: single-threaded host interpreter; between-`haeun_vm_step` intended safe point **[V/L]**; no SMP and no hardware atomicity claims **[L]**. See Figure F3 and Table T4.

## 7. Transactional policy application

Prepare ≠ commit **[V]**. Commit writes the policy word at the intended between-step boundary under serialized interpretation **[V/L]**. Rollback restores the **previous policy ID only**—not full VM/PCB/memory state **[O][L]** (`research/experiments/r43-rollback-verification.json`). Invalid targets leave policy unchanged in the recorded rejection case **[O]**.

## 8. Experimental methodology

Campaigns (existing artifacts only; no new runs invented):

| Campaign | Design | Artifact |
|----------|--------|----------|
| R4.1 standard | 6 workloads × 2 policies × 10 | prior reports; suites in freeze |
| R4.2 performance | 6×2×10=120 | `r42-performance-results.json` |
| R4.2 profiles | 6×5=30 recommendation cells | `r42-profile-verification.json` |
| R4.3 active apply | RR→FIFO, FIFO→RR + mirror | `r43-active-apply.json` |
| R4.3 safety/rollback | invalid + FI path | `r43-safety-rejection.json`, `r43-rollback-verification.json` |
| R4.3.2 worktree | detached rebuild/test | `docs/r432-reproduction-evidence.md` |

**Metrics:** guest `ready_wait` ticks (descriptive); Jain index computed but **identical** RR vs FIFO per workload in R4.2 tables **[O][L]**.  
**Not tested:** per-profile-cell live APPLY for all 30 cells (`NOT_TESTED_per_cell`) **[L]**.

## 9. Results

**Apply behavior [O]:** Both RR→FIFO and FIFO→RR reported `COMMITTED`; post-apply host-mirror matches 61/61 per direction in recorded R4.3 evidence (also echoed in author worktree `make test` log). Scope limited to those executions—not a universal guarantee.

**Recommendation surface [O]:** All 30 R4.2 profile cells recommended RR—evidence of catalog bias, not multi-objective policy diversity.

**Descriptive performance [O]:** RR mean `ready_wait` < FIFO on all six catalog workloads (Table T2). Deterministic HXF ⇒ n=10 often near-zero variance **[L]**.

**Reproduction [O][L]:** Author-host worktree: `CHAIN_EXIT_0; INDIVIDUAL_EXIT_CODES_NOT_CAPTURED`. Third-party: **NOT TESTED**.

## 10. Statistical limitations

n=10 deterministic repeats are **not** inferential significance **[L]**. No hypothesis tests were performed for RR vs FIFO superiority. Guest ticks ≠ wall-clock. Sample independence is only partial (fresh VM claimed; fixed images). Effect sizes are not primary claims.

## 11. Threats to validity

Internal: host mirror as behavioral proxy; thin rejection corpus; FI coverage limited.  
External: research ISA; six HXF workloads; two policies; Windows/MinGW author host.  
Construct: “autonomous” means automated evaluation, not unattended APPLY.  
Conclusion: same-host reproduction ≠ independent lab.

## 12. Discussion

AROS shows that *authority separation* and *fail-closed prepare* can be engineered as first-class interfaces between host optimization and guest policy state **[V][O]**. Performance tables motivate workload sensitivity but must not be read as general scheduling wins **[L]**. Compared with in-kernel learned policies [S13], our locus of optimization remains host-side by design.

## 13. Conclusion

Within a single-threaded research VM and a two-policy surface, HaeunOS AROS demonstrates host-assisted evaluate→recommend with safety-gated transactional apply, including recorded bidirectional apply with 61/61 mirror agreement **[V][O]**. Stronger claims—statistical superiority, fairness gains, SMP/atomicity, full-system rollback, unattended APPLY, third-party reproduction—are unsupported and withheld **[L]**.

## 14. References

S01–S15 as catalogued in `research/related-work/verified-sources.md` (access grades M/A/F preserved; see `research/paper/r45-citation-audit.md`). Internal evidence: R4.4 audit commit `3de73d4…`, freeze `386c3a1…`.
