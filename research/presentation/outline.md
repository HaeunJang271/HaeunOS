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
| 5 | Why AROS | Gap: measure vs *safe* runtime policy mutation | R4.5 manuscript §1 | 1:00 |
| 6 | Guest/host split | Host evaluates/recommends; guest policy word is separate authority | Manuscript; F1/F3 | 1:30 |
| 7 | SHADOW vs APPLY | SHADOW default → report only; APPLY opt-in → safety path | Manuscript; F1/F2 | 1:30 |
| 8 | Safety + transaction | UNKNOWN≠PASS; prepare ≠ commit; policy-ID rollback only | Manuscript; r43 safety docs | 2:00 |
| 9 | Results (scoped) | RR↔FIFO apply **Observed** 61/61 mirror; R4.2 catalog **descriptive** | Manuscript; r43/r42 artifacts | 2:30 |
| 10 | Limitations | Two policies; NOT TESTED items above; author-host rebuild only | r45 limitations; review readiness | 2:00 |
| 11 | Close | OS platform + honest AROS research boundary; next steps | — | 1:00 |

**Optional live demo (≤2 min, if time):** `build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer` after local `make` (not a third-party claim).

---

## Historical outline (R4.3 era, 12–15 min) — retained for provenance

The table below is the earlier AROS-only outline. Slide 10’s “dirty tree / HEAD worktree NOT TESTED” reflected the **R4.3 draft** moment; the current public tip is tag `v4.6.0` at `0b7fead…` with author-host worktree evidence recorded later. Prefer the 20-minute table above for external talks.

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
