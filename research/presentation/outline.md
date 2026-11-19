# HaeunOS 1.0 + AROS (FOSDEM-oriented, ~20 min)

**Presentation posture:** HaeunOS 1.0 is the primary topic; AROS is a **safety-centered research case study** on the same guest/host stack—not a second equal core track.

**Claim hygiene (use these labels aloud):** Implemented · Tested · Observed · Not tested · Not claimed.

**Hard limits (do not overclaim):** two policies only (RR=0, preemptive FIFO=1); 61/61 host-mirror matches are **Observed** (not general performance proof); no statistical superiority; third-party reproduction **NOT TESTED**; per-cell live APPLY **NOT TESTED**; no SMP; no hardware atomicity; no full VM rollback; no unattended APPLY.

| # | Title | Message | Evidence | Time |
|---|-------|---------|----------|------|
| 1 | Problem / goals | Custom 32-bit guest OS research; host never runs HAEUN natively | README; `docs/architecture.md` | 1:00 |
| 2 | HAEUN-32 + C17 VM | Interpreter VM; ISA/MMU boundaries | README; architecture | 2:00 |
| 3 | Isolation & faults | Process-v1 caps; user fault → ZOMBIE; containment | README contract; security/fault docs | 2:00 |
| 4 | Runtime surface | Desktop/apps/SDK path (demo optional) | Quick Start; SDK docs | 2:00 |
| 5 | Why AROS | Gap: measure vs *safe* runtime policy mutation | R4.8 manuscript §1 Introduction (`research/paper/r48-main-manuscript.md`) | 1:00 |
| 6 | Guest/host split | Host evaluates/recommends; guest policy word is separate authority | R4.8 §4–5; historical R4.5 F1/F3 (`research/figures/r45/f1-system-architecture.md`; `research/figures/r45/f3-safety-trust-boundary.md`) | 1:30 |
| 7 | SHADOW vs APPLY | SHADOW default → report only; APPLY opt-in → safety path | R4.8 §4–5; historical R4.5 F1/F2 (`research/figures/r45/f1-system-architecture.md`; `research/figures/r45/f2-eval-to-apply-pipeline.md`) | 1:30 |
| 8 | Safety + transaction | Single-threaded research VM; host-mediated prepare/validate/commit for a narrow guest policy word; UNKNOWN≠PASS; prepare ≠ commit; policy-ID rollback only (not full VM-state rollback); no SMP/HW atomicity claim | Manuscript §4–5; r43 safety docs | 2:00 |
| 9 | Results (scoped) | R4.3: RR↔FIFO apply **Observed** 61/61 mirror. R4.2: 120/120 successful runs; six workloads; RR lower ready-wait in all six; inferential tests **NOT_PERFORMED**; catalog cells **30/30 RR**. Gate B: `SELECT_RR` but measured `NOT_EVALUATED`. R47 C/D1/D2: all completed profiles `SELECT_RR`; no real FIFO select / `MEASURED_FIFO_BETTER`. This project is unrelated to Amiga AROS (AROS Research Operating System). | Manuscript (`r48`) §7; r43/r42/r47 artifacts | 2:30 |
| 10 | Limitations | Single-threaded research VM; no inferential testing; third-party reproduction **NOT TESTED**; no SMP/HW atomicity; policy-ID-only rollback (not full VM); R47: no real FIFO recommendation or `MEASURED_FIFO_BETTER`; W4 discontinued (not executed); two policies; author-host rebuild only | R4.8 manuscript §10 (`research/paper/r48-main-manuscript.md`) | 2:00 |
| 11 | Close | OS platform + honest AROS research boundary; next steps | — | 1:00 |

**Speaker notes (slide 9 only):** D1 preserved 300k step-cap failure (`INSUFFICIENT_TELEMETRY`); separately approved 800k confirmatory completed—do **not** treat 800k as making the 300k attempt successful. W4 discontinued and not executed.

**Optional live demo (≤2 min, if time):** `build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer` after local `make` (not a third-party claim).

---

## Historical outline (R4.3 era, 12–15 min) — provenance only; not part of the 20-minute talk

Supplementary historical material. Do **not** mix this table with the R4.8/R47 results above. Slide 10’s “dirty tree / HEAD worktree NOT TESTED” reflected the **R4.3 draft** moment; the current public tip is tag `v4.6.0` at `0b7fead…` with author-host worktree evidence recorded later. Prefer the 20-minute table above for external talks.

| # | Title | Message | Evidence | Time |
|---|-------|---------|----------|------|
| 1 | Problem | Unsafe ad-hoc policy change | — | 0:40 |
| 2 | RQ | Host-assisted evaluate→validate→apply | RQ | 0:40 |
| 3 | Existing work | MAPE, Rainbow, K42 | S01–S06 | 1:00 |
| 4 | Architecture | Modular monolith; host vs guest | figures | 1:00 |
| 5 | Host-assisted opt | 5 profiles; SHADOW | r36/r42 | 0:50 |
| 6 | Safety | UNKNOWN≠PASS | r31–r32 | 1:00 |
| 7 | Transactions | Policy-ID rollback | r32/r43 | 1:00 |
| 8 | Selection proof | Mirror vs curr, not ID alone | r43 trace | 1:30 |
| 9 | Active apply | RR↔FIFO COMMITTED + matches | r43 apply | 1:30 |
| 10 | Reproducibility | R4.3-era git baseline gaps; 3rd party NOT TESTED | historical R4.3 notes | 1:00 |
| 11 | Limitations | Catalog; two policies; no SMP | — | 0:50 |
| 12 | Contributions | C2b upgrade; honest gaps | registry | 0:40 |
