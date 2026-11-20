# HaeunOS AROS: An Autonomous Runtime Policy Optimization Framework for Safe Operating System Reconfiguration

*R4.3 external reproduction & active apply revision (historical draft).*

**Document status (R4.6.4 clarification):** This file is the **R4.3** draft narrative. It is **not** the current publication manuscript. The R4.5 publication manuscript is `research/paper/r45-main-manuscript.md`.

## Abstract

Operating systems often fix scheduling policy early, leaving a gap between measurement and safe runtime change. HaeunOS AROS is a research prototype on a modular monolithic HAEUN-32 guest. Round-robin and preemptive FIFO are selected through an explicit policy word. A host-assisted framework evaluates candidates and applies changes only through a fail-closed validator and transaction protocol (SHADOW default; UNKNOWN ≠ PASS). R4.3 adds behavioral evidence that after RR↔FIFO commits, observed `curr` matches a host mirror of the guest selection rules (61+61 events, zero mismatches), and refreshes safety-rejection and policy-ID-only rollback checks. **At the time this R4.3 draft was written** (before the R4.3.1 source freeze), an immutable Git/HEAD worktree baseline was not established because the working tree still contained uncommitted research sources; third-party reproduction remains NOT TESTED. Those research sources were subsequently committed at R4.3.1 (`386c3a1517c49be8320c6ea263e726a6c006bb53`), with same-host worktree rebuild evidence recorded in R4.3.2. Catalog performance comparisons from R4.2 are prior descriptive results, not re-asserted as new R4.3 runs. No SMP, hardware atomicity, or full VM rollback is claimed.

## 1. Introduction

Can a host-assisted framework safely evaluate, recommend, validate, and apply runtime scheduling policies while preserving documented kernel invariants?

## 2. Research Questions

As in `docs/r40-hypotheses.md` (host-assisted AROS terminology).

## 3. Background and Related Work

MAPE/self-adaptive [S01,S02,S05], Rainbow [S06], K42 reconfiguration [S03,S04], lottery/Exokernel [S09,S12], Jain [S10], repeatability [S11]. No priority claims.

## 4. HaeunOS Architecture

Modular monolithic guest + host tooling. Optimizer/planner/robust campaigns are host-side.

## 5. Scheduler Policy Abstraction

RR and preemptive FIFO only (`0xA474`).

## 6. Runtime Safety Validation

`pval`; UNKNOWN rejects apply.

## 7. Transactional Reconfiguration

Between `haeun_vm_step` under single-threaded interpretation. Rollback restores policy ID only.

## 8. Autonomous Planning and Optimization

Five profiles; SHADOW default. R4.2 catalog: all profiles recommended RR (prior).

## 9. Experimental Methodology

R4.3 active verification uses `r12_pair` / `fcfs/r12_pair`, eligible≥2, seq OK, host mirror vs observed `curr`.

## 10. Results

RR→FIFO and FIFO→RR COMMITTED; post-apply selection matches 61/61 each. Invalid/FI rejection paths exercised. At R4.3 draft time, HEAD worktree was **NOT TESTED**; same-host worktree verification followed at R4.3.1/R4.3.2. Third-party remains **NOT TESTED**.

## 11. Reproducibility

External package scripts exist. At R4.3 draft time, a dirty tree blocked an immutable baseline; the R4.3.1 freeze later established a committed baseline on the author host. Third-party execution is still required for any third-party reproduction claim.

## 12. Threats to Validity

At R4.3 draft time: dirty git / uncommitted sources (later frozen at R4.3.1); deterministic HXF; post-apply-only selection samples in R4.3 traces; same host.

## 13. Limitations

Two policies; policy-ID rollback; no SMP; R4.3-era uncommitted sources (superseded by R4.3.1 freeze for author-host baseline); no 3rd-party lab.

## 14. Conclusion

R4.3 closes the behavioral-apply evidence gap. This draft did not yet claim a clean Git baseline or third-party success; the subsequent R4.3.1 freeze and R4.5 manuscript supersede the draft’s baseline-status wording without inventing third-party reproduction.

## 15. References

S01–S15 as in `research/related-work/verified-sources.md`; HaeunOS docs through R4.3.
