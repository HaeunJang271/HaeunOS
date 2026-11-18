# R4.5 Reviewer Readiness

**Package status target:** PUBLICATION_PACKAGE_READY_WITH_LIMITATIONS  
**Audit tip:** `3de73d4ee0649d990bc0da5904872f24767fe53e`  
**Implementation:** `386c3a1517c49be8320c6ea263e726a6c006bb53`

## Novelty positioning

Argue **composition**: host-assisted evaluate/recommend + UNKNOWN≠PASS prepare + transactional policy-ID apply + ready_seq FIFO on a research VM. Do **not** claim each piece is unprecedented versus Rainbow/K42/HotOS’21 learned kernels. Cite S01–S15 with preserved M/A/F grades.

## Reproducibility

- Same-host worktree: documented chain exit 0.
- Third-party: **NOT TESTED** — state upfront; provide `research/reproduction/r45/`.
- Exclude absolute-path env/logs from the artifact story.

## Statistical validity

- Descriptive catalog only; n=10 deterministic ≠ significance.
- Jain identical ⇒ no fairness-improvement claim.
- Separate safety/apply results from performance tables.

## Safety claims

- Lead with authority separation and prepare/commit.
- Rollback = policy ID; safe point = single-thread between-step convention.
- Never equate to SMP or HW atomicity.

## Policy diversity

- Only RR and preemptive FIFO.
- 30/30 RR recommendations: report as limitation, not multi-objective success.
- 30-cell live APPLY: NOT_TESTED_per_cell.

## External validity

Research ISA + six HXF images + author Windows/MinGW host. No Linux production generalization.

## Likely objections → response pointers

| Objection | Response location |
|-----------|-------------------|
| Only two policies | Manuscript §2 non-contributions; Table T1 |
| Always RR in profiles | Results §9; Figure F4 |
| Not autonomous | Define eval autonomy; SHADOW default |
| Perf superiority | Statistical limitations §10; Class C |
| Rollback oversold | Safety §6–7; T4 |
| Atomicity | F3; PartIAL safe-point grade |
| Independent repro | R45 README; NOT TESTED |
| Novelty weak | Qualified distinction; citation audit |

## Remaining blockers for stronger venues

Not required for limited publication story, but would strengthen: third-party dry-run; FIFO-win workload; stochastic loads; richer FI matrix; wall-clock metrics.
