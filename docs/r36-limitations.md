# R3.6 Limitations

- Candidate space: RR + preemptive FIFO only (no MLFQ/SRTF/priority yet).
- Descriptive CI ≠ hypothesis test; small-N not significance.
- Guest ticks ≠ wall-clock.
- last_rec keeps last SELECT across workloads (per-row results authoritative).
- No SMP; no full-VM rollback; not ML.
