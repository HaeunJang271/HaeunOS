# R4.5.2 Minor Issue Resolution

**Status: READY_FOR_FINAL_FREEZE**  
**HEAD (unchanged):** `3de73d4ee0649d990bc0da5904872f24767fe53e`  
**Edited file:** `research/figures/r45/f1-system-architecture.md` only  
**Manuscript / F2–F5 / R4.4 / R4.5.1 audits:** not modified

## Original F1 issue (R4.5.1)

In `research/figures/r45/f1-system-architecture.md`, the edge

`OPT -->|SHADOW: no write| SAF`

visually attached SHADOW to the Safety prepare (`SAF`) node, which could be misread as SHADOW entering the authorization/prepare path even though the label said “no write”.

## Exact correction

1. Removed `OPT --> SAF` under SHADOW.
2. Added host-side sink `REP[SHADOW report only]` with `OPT -->|SHADOW default| REP`.
3. Kept `OPT -->|APPLY opt-in| SAF` then `SAF -->|prepare then| TXN` then policy-ID write.
4. Caption updated to state SHADOW ends without `0xA474` write; APPLY alone enters prepare→commit.

## Before / after Mermaid relationships

| Relationship | Before | After |
|--------------|--------|-------|
| SHADOW | OPT → SAF (labeled no write) | OPT → REP (report only) |
| APPLY | OPT → SAF → TXN → POL | unchanged intent; prepare edge labeled `prepare then` |
| Automatic APPLY | none | none |
| SMP / HW atomicity | none | none |
| Full-system rollback | none | none (caption: policy ID only if rollback) |

## Supporting evidence (no new architecture invented)

| Source | Support |
|--------|---------|
| `src/opt_cli.c` | SHADOW/OPTIMIZE modes vs explicit `HAEUN_OPT_MODE_APPLY` calling `haeun_opt_apply` |
| `docs/r44-safety-invariant-audit.md` | SHADOW default; APPLY opt-in; prepare≠commit |
| `research/paper/r45-main-manuscript.md` §5 | SHADOW evaluates without mutating policy; APPLY opt-in |
| `research/figures/r45/f2-eval-to-apply-pipeline.md` | SHADOW → Report only; APPLY → prepare → commit |

`REP` is the same conceptual sink as F2’s “Report only”, not a new runtime subsystem.

## Confirmation

No unsupported execution path added. No kernel/host/test/experiment edits. No staging/commit/push.
