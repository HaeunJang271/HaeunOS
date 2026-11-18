# R3.5 Feedback Model

Deterministic, explainable, rule-based (not ML).

## Outcomes

| Outcome | Meaning |
|---------|---------|
| IMPROVED | weighted valid objectives show net improvement above thresholds |
| DEGRADED | net regression above thresholds |
| NEUTRAL | comparable, no decisive improve/regress |
| INCONCLUSIVE | incomparable, insufficient samples, or no committed change |

## Gates applied to R3.3 recommendations

1. Cooldown remaining → KEEP_CURRENT
2. Previous INCONCLUSIVE → NO_DECISION
3. Consecutive DEGRADED toward previous policy → KEEP_CURRENT (anti-oscillation)

Ambiguous evidence → NO_DECISION. Never force a policy selection.

## Execution rule

Only ADAPTIVE + execute + RECOMMEND_CHANGE + safety OK → R3.2. Feedback never writes `0xA474` directly.
