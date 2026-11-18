# R3.6 Architecture

Host-only autonomous policy optimization.

```
Characterize → Candidates{RR,FIFO} → Experiment(R2.2)
→ Stats(+CI n≥2) → Pareto → Select(profile)
→ Recommendation
→ APPLY only: R3.1 → R3.2 → R3.4; Feedback R3.5
```

| Mode | Behavior |
|------|----------|
| SHADOW | report / load state; no explore required |
| OPTIMIZE | offline experiments; **never** mutates active policy |
| APPLY | explicit; fresh R3.1→R3.2 |

Default profile `balanced`, default reps `3` (tests may use 2).
