# R4.1 Research Differentiation

## Shared ideas

MAPE-style control (S01–S02, S05–S06), online OS reconfiguration (S03–S04), flexible scheduling (S09, S12), fairness metrics (S10), and artifact/repeatability norms (S11).

## Similar architectures

Rainbow’s external control loop (S06) and K42 hot-swap (S03–S04) are closest architectural relatives: adaptation logic separable from managed resource; quiescence/safe points matter.

## Different implementation choices

| Choice | HaeunOS | Typical prior |
|--------|---------|---------------|
| Managed unit | Scheduler policy word | Components / architecture models / tickets |
| Optimizer location | Host research VM tooling | In-system or architecture manager |
| Apply default | SHADOW | Often auto-adapt |
| Failure mode | UNKNOWN rejects commit | Varies |
| Evidence | Replay + sensitivity + verify enums | Often informal eval |

## Potentially novel *combination* (not “first”)

Auditable pipeline coupling: guest policy abstraction + host multi-objective evaluation + fail-closed validation + explicit txn + recommendation evidence classifiers on a reproducible HXF catalog.

## Unproven novelty claims

- Superior scheduling performance vs Linux/CFS/lottery — **NOT TESTED**.
- Broader than MAPE-K — **false**; HaeunOS is a specialized instance.
- Production autonomous OS — **not claimed**.

## Research gaps addressed (modestly)

1. Explicit separation of recommendation and apply in an OS *scheduling policy* setting.
2. Executable claim–evidence linkage for a research prototype.
3. Small-N → larger-N progression (smoke→standard) with honest statistical limits.
