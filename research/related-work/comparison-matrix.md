# Comparison Matrix (HaeunOS vs Related Work)

Legend: Y=yes, P=partial, N=no/not primary, U=UNKNOWN from available text.  
HaeunOS column reflects **implemented** prototype behavior, not aspirational features.

| Dimension | HaeunOS AROS | Autonomic/MAPE (S01–S02) | Rainbow (S06) | K42 hot-swap (S03–S04) | Lottery/Exokernel (S09/S12) | HotOS RMT (S13) |
|-----------|--------------|--------------------------|---------------|------------------------|-----------------------------|-----------------|
| Policy abstraction | Y (RR/FIFO ID) | U | P (arch tactics) | Y (components) | Y | Y (tables) |
| Runtime adaptation | Y (txn apply) | Vision | Y | Y | P | Y (vision) |
| Safety validation gate | Y (pval; UNKNOWN≠PASS) | U | P (constraints) | P (quiescence) | N/U | Y (verifier) |
| Explicit commit txn | Y (R3.2) | U | P | Y (swap protocol) | N | P |
| Rollback | P (policy ID only) | U | U | P (state transfer) | N | U |
| Autonomous planning | Y (host planner/opt) | Vision | Y | P | N | Y (ML) |
| Multi-objective opt | Y (5 profiles + Pareto) | U | P | N/U | N | P |
| Reproducible experiments | Y (presets/hash/replay) | N | U | U | U | U |
| Sensitivity analysis | Y (weight perturb) | N | U | N | N | U |
| Evidence traceability | Y (claim/test/exp matrix) | N | U | U | N | U |
| In-kernel optimizer | N (host-assisted) | U | N (external) | P | N | Y |
| SMP / production scale | N | Vision | P | Y (research OS) | P | Vision |

**Interpretation:** Shared ideas are common; HaeunOS’s documented combination emphasizes *host-assisted multi-objective recommendation + fail-closed scheduling-policy txn + replay/sensitivity evidence* on a research ISA. Comparative *performance superiority* vs these systems is **NOT TESTED** (different platforms).
