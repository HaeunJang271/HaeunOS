# R4.5.1 Publication Consistency Audit

**Status: READY_WITH_MINOR_ISSUES**  
**Mode:** READ-ONLY (R4.5 artifacts not modified)  
**HEAD:** `3de73d4ee0649d990bc0da5904872f24767fe53e`  
**Refs verified:** `386c3a1…bb53`, `735013e…e288`, `3de73d4…e53e` (truncated `…fe53` resolves to same commit)

## Inventory audited (13 files)

See `docs/r451-final-file-inventory.md`.

## Audit 1 — Manuscript consistency

| Check | Result | Reference |
|-------|--------|-----------|
| RQ matches R4.4 | PASS | `r45-main-manuscript.md` L21 == `docs/r44-final-research-audit.md` L11 |
| Abstract ↔ contributions ↔ results ↔ conclusion | PASS | Abstract L15; §2 L27–35; §9 L84–90; §13 L109 — same A/B/C gates |
| Quantitative statements sourced | PASS w/ note | See evidence matrix; 61/61 via R4.3 audit/log lineage, not fields inside `r43-active-apply.json` |
| Limitations visible | PASS | Abstract L15; Non-contributions L35; §10–11; conclusion L109 |
| No invented implementation | PASS | Components map to freeze paths named in §4–7 |
| No invented venue/page/DOI requirements | PASS | Header L3–7; no venue claim |

## Audit 2 — Numerical evidence

All checked values match existing artifacts (matrix). No recalculation performed beyond read-back.

## Audit 3 — Figures

| Figure | Verdict | Notes |
|--------|---------|-------|
| F1 | MINOR | L26 routes `SHADOW: no write` into `SAF` node; caption clarifies no write. Prefer F2’s SHADOW→Report for clarity. Does **not** claim APPLY. |
| F2 | PASS | SHADOW/APPLY, prepare≠commit, policy-ID rollback explicit (L11–16) |
| F3 | MINOR | Floating `Note1/2/3` nodes (L21–23) are caption-like; no SMP/atomicity implied as supported |
| F4 | PASS | Open items include NOT TESTED / NOT CLAIMED |
| F5 | PASS | Class C listed as cannot-claim |

## Audit 4 — Tables T1–T6

PASS against R4.2 stats JSON, R4.3 JSON, R4.4 matrices. T2 means/Jain match `r42-statistics.json` workload0+docs table.

## Audit 5 — Citations

PASS: S01–S15 IDs consistent with `verified-sources.md`; M/A/F preserved in `r45-citation-audit.md`. Manuscript uses S01–S15 as background/positioning only; novelty marked `[H]` / qualified (manuscript L41). No invented quotes.

## Audit 6 — Reproducibility

PASS: checkout `386c3a1…`; env matches R4.3.2 recorded host; commands accurate; artifact paths exist; third-party **NOT TESTED**; expected vs guaranteed distinguished in README.

## Audit 7 — Prohibited claims

Package-wide search: prohibited themes appear only as **non-claims / limitations / Class C**, never as established results. PASS.

## Audit 8 — Git integrity

| Check | Result |
|-------|--------|
| HEAD | `3de73d4…e53e` |
| Staged | empty |
| Impl/kernel/host/tests/experiments dirty | none |
| EXCLUDE 12 | still untracked |
| R4.5 edits this audit | none (audit docs only) |

## Minor issues (non-blocking)

1. **F1 SHADOW→SAF edge** (`research/figures/r45/f1-system-architecture.md` L26) — clarity.  
2. **61/61 primary source** is R4.3 audit / worktree log text, while `r43-active-apply.json` records COMMITTED transitions without match counts — manuscript already scopes to “recorded R4.3 evidence” (`r45-main-manuscript.md` L84); matrix documents dual sourcing.  
3. User prompt SHA omits final `e`; package uses full SHA correctly.

## Decision

**READY_WITH_MINOR_ISSUES** — safe to request commit approval for R4.5 package; optional figure/caption polish not required for honesty of claims.
