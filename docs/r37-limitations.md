# R3.7 Limitations

- Catalog workloads have no RNG; seed is configuration fingerprint.
- Parametric workload sweeps NOT_SUPPORTED.
- Descriptive CI only; small-N (smoke=3) is not significance.
- Research preset may truncate experiment records at HAEUN_RB_REC_MAX.
- Live RR→FIFO may still UNKNOWN under seq/wrap rules (fail closed).
