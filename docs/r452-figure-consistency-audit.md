# R4.5.2 Figure Consistency Audit

**After F1-only correction. F2–F5 untouched.**

## F1–F5 term consistency

| Term | F1 (after) | F2 | F3 | F4 | F5 | Consistent? |
|------|------------|----|----|----|----|-------------|
| SHADOW | default → report only, no policy write | → Report only | (via opt path) non-authorizing | open items N/A | B1 not unattended APPLY | YES |
| APPLY | explicit opt-in → SAF | explicit → prepare | gated authorize | 30-cell live NOT TESTED called out | A path via prepare/commit | YES |
| Recommendation | OPT node | Recommend policy | must not authorize alone | — | A1/A2 | YES |
| SAF / prepare | Safety prepare; edge `prepare then` | `haeun_recfg_prepare` | prepare UNKNOWN≠PASS | — | A3/A4 | YES |
| Commit | TXN after prepare | `haeun_recfg_commit` | commit between-step convention | — | A4 | YES |
| Rollback | caption: policy ID only | rollback policy ID only | Not full VM rollback | reject/rollback ID covered | B3 / C4 forbidden full | YES |

No genuine contradiction requiring F2–F5 edits.

## Manuscript consistency

`research/paper/r45-main-manuscript.md` **not modified**. F1 now aligns with §5 SHADOW/APPLY wording; no claim change required.

Preserved semantics: SHADOW default; APPLY opt-in; recommendation ≠ authorization; prepare ≠ commit; rollback = policy ID; safe-point = single-thread between-step (F3 / manuscript), not implied by F1 as HW atomicity.

## Safety semantics

- No automatic unattended APPLY path in any figure.
- No SMP / hardware atomicity presented as supported.
- No full-system rollback path.

## Evidence restrictions (preserved from R4.5.1)

- 61/61: recorded R4.3 evidence/logs only (not apply JSON field).
- 30-cell live APPLY: NOT_TESTED_per_cell (F4).
- Third-party: NOT TESTED.
- n=10 deterministic: descriptive only.
- Jain RR=FIFO: no fairness-improvement claim.

## Remaining issues

None material. Optional future polish (not required): render F3 note nodes as captions only for Mermaid lint aesthetics.

## Final status

**READY_FOR_FINAL_FREEZE**
