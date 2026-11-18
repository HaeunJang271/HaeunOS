# R3.7 Architecture

Host-side validation layer over R2.2–R3.6.

```
Config/Preset → Repeated Runner → Replay Compare
→ Reliability → Sensitivity → Robustness notes
→ Verify R3.6 recommendations → Aggregate
→ (optional) R3.2 reconfig stress on live VM
```

OPTIMIZE/APPLY remain separate. R3.7 never bypasses R3.1/R3.2.
No new scheduling policies.
