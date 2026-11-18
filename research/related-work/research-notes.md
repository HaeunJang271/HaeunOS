# Research Notes (R4.1)

## Shared ideas with prior work

- **MAPE-K / autonomic loops (S01–S02, S05–S06):** Monitor→Analyze→Plan→Execute over Knowledge is the conceptual ancestor of HaeunOS host tooling (telem→eval/opt→planner→reconfig).
- **External control (S06 Rainbow):** Separating adaptation logic from the managed system matches host-assisted optimization (optimizer outside guest kernel).
- **Online OS reconfiguration (S03–S04):** Hot-swapping / interposition shows runtime component change is a long-standing OS research theme.
- **Flexible scheduling (S09, S12):** Policy flexibility and application-level control predate HaeunOS’s RR/FIFO abstraction.
- **Fairness metrics (S10):** Jain index is standard; HaeunOS reuses it as an evaluation signal.
- **Reproducibility culture (S11):** Artifact and repeatability expectations motivate R3.7/R4.1 presets and export.

## Differences (careful wording)

HaeunOS does **not** claim to be “first” at adaptation, hot-swap, or multi-objective scheduling. Relative positioning:

1. **Narrow managed resource:** scheduling *policy ID* in a research guest (HAEUN-32), not full component hot-swap (K42) or architecture-model Rainbow gauges.
2. **Explicit fail-closed safety gate:** UNKNOWN ≠ PASS before commit; documented single-threaded between-step safe point.
3. **Recommendation ≠ apply:** SHADOW default; APPLY opt-in through txn path only.
4. **Evidence classifiers:** replay / sensitivity / verify statuses tying recommendations to experiment fingerprints.
5. **Host-assisted boundary:** optimization never writes guest scheduler memory directly.

## Gaps in literature vs HaeunOS claims

| Gap | Notes |
|-----|-------|
| Safety-gated *policy-word* txn + host multi-objective opt + evidence verify in one auditable stack | Combination may be distinctive as *integration*; individual pieces exist |
| Formal SMP-safe reconfiguration | Not claimed; literature (S15) shows how hard OS reconfig is |
| Learned in-kernel policies (S13) | HaeunOS uses classical RR/FIFO + host scoring, not ML RMT |
| Broad production workloads | HaeunOS fixed HXF catalog only |

## Attribution discipline

Do not attribute “MAPE-K” terminology inventorship beyond what S01/S02 support (autonomic self-management vision). Salehie survey (S05) is used for landscape taxonomy, not as proof of HaeunOS novelty.
