# R4.5.1 Evidence Traceability Matrix

**Rule:** values read from existing artifacts only; no silent fills.

| Claimed value / statement | Where used (R4.5) | Source artifact | Field / section | Match? |
|---------------------------|-------------------|-----------------|-----------------|--------|
| RQ text | `r45-main-manuscript.md` L21 | `docs/r44-final-research-audit.md` L11 | Research question blockquote | YES |
| Implementation SHA `386c3a1…bb53` | manuscript L4; r45 README; baseline.json | git object | `git rev-parse` / freeze | YES |
| Docs tip `735013e…` | manuscript L5; README | git log | R4.3.2 commit | YES |
| Audit tip `3de73d4…e53e` | manuscript L6; README | git HEAD | R4.4 commit | YES |
| Policies RR=0, FIFO=1 | manuscript Abstract; T1 | `kernel/process/proc_policy.s`; R4.4 C09 | policy IDs | YES |
| 6 workloads | manuscript; T2/T3 | `r42-statistics.json` / performance docs | workloads list length 6 | YES |
| n=10 | manuscript §10; T2 | `r42-statistics.json` | `n_per_cell`: 10; per-policy `n`: 10 | YES |
| 120 runs (6×2×10) | Abstract; T3 | `r42-performance-results.json` | `planned_runs`/`success_runs`/`fail_runs` = 120/120/0 | YES |
| ready_wait RR/FIFO means (e.g. cpu 749/812) | T2 | `r42-statistics.json` | `workloads[].RR/FIFO.ready.mean` + `docs/r42-performance-analysis.md` L20–25 | YES |
| Jain RR=FIFO per row | T2; manuscript L79 | same stats JSON `jain.mean` | identical pairs | YES |
| inferential NOT_PERFORMED | manuscript §10 | `r42-statistics.json` | `inferential_tests` | YES |
| 30 profile cells, all RR | manuscript L86; T3 | `r42-profile-verification.json` | `cells` len=30; all `recommended_policy==0` | YES |
| `NOT_TESTED_per_cell` | manuscript L80; T3 | profile JSON `apply_path` | string present on cells | YES |
| RR↔FIFO COMMITTED | manuscript L84; T3 | `r43-active-apply.json` | `txn":"COMMITTED"` both labels | YES |
| 61/61 matches each direction | Abstract L15; §9 L84; F4/F5 | `docs/r43-active-apply-audit.md` L9–10; `docs/r432-reproduction-evidence.md`; worktree log lines | audit table + log text | YES (not a field in apply JSON) |
| Invalid policy blocked | manuscript L64 | `r43-safety-rejection.json` | `blocked:true`, `pol:0` | YES |
| Rollback policy_id_only | manuscript L64; T4 | `r43-rollback-verification.json` | `scope":"policy_id_only"`, `policy_id_restored:true` | YES |
| Chain exit 0 / individual NOT CAPTURED | manuscript L90; README | `docs/r432-reproduction-evidence.md` | Exit status string | YES |
| Third-party NOT TESTED | throughout | R4.4 / R4.3.2 docs | explicit | YES |
| UNKNOWN≠PASS / prepare≠commit | manuscript §6–7; F2/F3 | `src/reconfig_txn.c`; R4.4 safety audit | code + audit grades | YES |
| SHADOW default / APPLY opt-in | manuscript L53; F2 | `src/opt_cli.c` paths; R4.4 | VERIFIED | YES |
| S01–S15 access grades | citation audit | `research/related-work/verified-sources.md` | Access column | YES |

## Gaps / clarifications (not missing evidence)

| Item | Clarification |
|------|----------------|
| 61/61 | Cite R4.3 active-apply **audit** + reproduction log; apply JSON alone insufficient for match counts |
| F1 SHADOW→SAF | Diagram clarity only; see consistency audit |
