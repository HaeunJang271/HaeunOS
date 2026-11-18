# R4.4 Claim–Evidence Matrix

**Baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Prior registries:** `research/evidence/r43-claims-registry.*`, `docs/r40-claim-evidence-matrix.md`

## Legend

- **A** Can claim  
- **B** Can claim with qualification  
- **C** Cannot claim  

Grades: VERIFIED / OBSERVED / PARTIAL / INSUFFICIENT / NOT TESTED / CONTRADICTED

## Claims (≥10)

| ID | Exact claim | Code | Test | Experiment | Related-work distinction | Grade | Class | Limits | Publication-safe wording |
|----|-------------|------|------|------------|--------------------------|-------|-------|--------|--------------------------|
| C01 | Host AROS pipeline evaluate→recommend exists | opt/planner/feedback | r33–r37,r40–r42 | R4.1/R4.2 runs | External loop akin to Rainbow; OS-policy focus | VERIFIED | A | Research VM | “Host-assisted evaluation and recommendation over guest telemetry.” |
| C02 | Optimizer cannot write policy alone | `opt_apply`→`recfg` | r32,r36,r43 | apply JSON | Separation vs in-kernel learners (S13) | VERIFIED | A | Host trust | “Recommendations require a separate transactional safety path.” |
| C03 | UNKNOWN≠PASS at prepare | `reconfig_txn.c` | r31/r32 | rejection JSON | Explicit gate uncommon in adaptive sched papers | VERIFIED | A | Gate set finite | “Prepare rejects UNKNOWN/FAIL invariant outcomes.” |
| C04 | Prepare and commit are distinct | reconfig APIs | r32,r43 | active apply | Transactional reconfiguration (S04 lineage) | VERIFIED | A | Host-mediated | “Policy mutation is two-phase (prepare then commit).” |
| C05 | Safe point is between VM steps | comments+API | r32 | — | Not HW atomicity | PARTIAL | B | Convention | “Commits are intended at between-`haeun_vm_step` boundaries under a single-thread interpreter.” |
| C06 | Active RR↔FIFO apply changes selection behavior | policy_verify mirror | r43 | 61/61 matches | Behavioral proof beyond word write | OBSERVED | A/B | Mirror≠full guest proof | “After apply, host mirror of policy_select matched observed post-apply selections in recorded runs (61/61).” |
| C07 | Invalid applies blocked | prepare gates | r43 | safety JSON | — | OBSERVED | A | Thin corpus | “Invalid targets leave policy unchanged in tested cases.” |
| C08 | Rollback restores policy ID only | `haeun_recfg_rollback` | r32,r43 | rollback JSON | Not full checkpoint | OBSERVED | B | Scope narrow | “Rollback restores the prior policy identifier, not full VM state.” |
| C09 | Two policies only (RR, preemptive FIFO) | `proc_policy.s` | r20+ | — | Narrow vs CFS/lottery (S09) | VERIFIED | B | Diversity low | “Evaluation uses two guest policies: RR and preemptive FIFO.” |
| C10 | Catalog RR lower ready_wait than FIFO | eval metrics | r42 | 120 runs | Not vs Linux/Rainbow | OBSERVED | B | Deterministic catalog | “On six fixed HXF workloads, descriptive mean ready_wait favored RR.” |
| C11 | Fairness improved by AROS | Jain equal RR/FIFO | r42 | stats JSON | S10 used as metric only | CONTRADICTED if claimed | C | No delta | Do not claim fairness improvement. |
| C12 | Autonomous closed-loop always-on control | feedback exists | r35 | SHADOW-heavy | Autonomic MAPE (S01) | PARTIAL | B/C | Human/CLI gated APPLY | “Automation evaluates; apply remains explicit opt-in.” |
| C13 | Kernel invariants preserved under apply | pval+prepare | r31–r34,r43 | — | — | OBSERVED/VERIFIED | B | Defined invariant set | “Documented invariants are checked before commit; PASS required for prepare.” |
| C14 | Same-host reproducible build/test | Makefile | make test | R4.3.2 worktree | S11 norms | OBSERVED | B | Chain exit only | “Detached worktree rebuild of freeze SHA exited 0 on the author host.” |
| C15 | Third-party reproducible | package docs | — | — | S11 | NOT TESTED | C | — | Do not claim. |
| C16 | Inferential RR superiority | — | — | — | — | INSUFFICIENT | C | Deterministic n | Do not claim statistical significance. |
| C17 | Generalizes beyond catalog | — | — | — | — | NOT TESTED | C | — | Do not claim. |
| C18 | SMP-safe / HW-atomic apply | — | — | — | — | NOT TESTED | C | Explicit non-goal | Do not claim. |

## Related-work novelty (qualified)

Plausible distinction (B): **host-assisted optimization + UNKNOWN≠PASS + transactional policy-ID apply + ready_seq FIFO ordering** on a research ISA VM.  
Not proven novel vs every adaptive OS (needs careful related-work prose; S01–S15 support positioning, not automatic novelty).
