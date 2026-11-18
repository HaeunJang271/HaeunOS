# R4.4 Reviewer Attack Simulation

**Audience:** OS / systems conference reviewers  
**Baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`

Severity: H = high reject risk, M = major revision, L = clarification.

| # | Question | Sev | Existing evidence | Answerable now? | Missing | Extra experiment | Paper strategy |
|---|----------|-----|-------------------|-----------------|---------|------------------|----------------|
| 1 | Why only two scheduling policies? | H | `proc_policy.s` RR/FIFO | Partially | More policies | Add ≥1 policy or justify minimal pair | Frame as minimal controllable policy surface |
| 2 | Why RR in every profile cell? | H | 30/30 RR in `r42-profile-verification.json` | Yes honestly | Counterexample cells | Stress profiles / workloads where FIFO wins | Report collapse; don’t claim multi-objective diversity |
| 3 | Is the optimizer actually autonomous? | H | SHADOW default; APPLY opt-in | Yes | Continuous unsupervised apply | Optional guarded auto-apply study | Define “autonomous evaluation” ≠ unsupervised commit |
| 4 | Improve performance or only preserve safety? | H | Descriptive ready_wait; safety OBSERVED | Split answer | Causal apply A/B on live policy | Before/after under apply | Separate safety claims from descriptive perf |
| 5 | Statistically meaningful evaluation? | H | n=10 deterministic | Limited | Inferential design | Stochastic loads / independence | Emphasize engineering evidence, not p-values |
| 6 | Rollback = full system rollback? | H | `scope=policy_id_only` | Yes | Checkpointing | Optional snapshot rollback | Explicit narrow semantics |
| 7 | Safe point guarantees atomicity? | H | Between-step convention | Partially | Formal concurrency model | Multi-thread host stress (if ever) | “Serialized interpreter,” not HW atomic |
| 8 | Independently reproducible? | H | Same-host worktree | No for 3rd party | External lab | Blind package run | Claim same-host; list 3rd party as future |
| 9 | Why host-assisted vs in-kernel optimizer? | M | Boundary docs; S13 contrast | Yes | Head-to-head | Microbench overhead | Argue isolation & UNKNOWN≠PASS mediation |
| 10 | What is genuinely novel? | H | S01–S15 matrix | Qualified | Stronger differentiation study | Position table vs K42/Rainbow/HotOS’21 | Novelty = composition + gates, not each piece |
| 11 | Guest ticks vs wall-clock? | M | R4.2 analysis | Yes | Wall-clock study | Dual metrics | Disclose units |
| 12 | Host mirror ≠ kernel behavior? | M | 61/61 mirror | Partially | Deeper guest instrumentation | More selection probes | State mirror as behavioral proxy |
| 13 | Failure injection coverage thin? | M | FI AFTER_WRITE | Partially | More FI points | Expand FI matrix | Report tested points only |
| 14 | PCB/telem trustworthiness? | M | telem/pval | Partially | Adversarial guest | Fault guest tests | Threat model: cooperative research guest |
| 15 | Feedback loop closed in eval? | M | r35 exists; campaigns often SHADOW | Partially | Always-on adaptive campaign | Long-horizon feedback | Don’t oversell closed-loop |
| 16 | Overhead of host tooling? | M | host clock notes | Weak | Systematic overhead | Instrument tooling vs guest | Separate tooling wall time |
| 17 | Fairness claims? | M | Jain identical | Yes (reject claim) | — | — | Remove fairness-improvement claims |
| 18 | Ready_seq wrap handling? | L | wrap sticky UNKNOWN | Yes | Long-run wrap | Stress wrap | Document UNKNOWN path |
| 19 | Artifact availability? | M | freeze SHA + scripts | Partially | Public CI | GitHub artifact badge | Ship freeze SHA + reproduce.md |
| 20 | Threat to external validity? | H | 6 HXF catalog | Yes | Broader apps | New workloads | Limit external validity paragraph |
| 21 | Does prepare race with guest? | M | single-thread step | Partially | Concurrent host threads | — | State non-preemptive host |
| 22 | Policy word sole mutable state? | M | txn sole writer design | Mostly | Formal sole-writer proof | Static check | Claim design + tests |
| 23 | Why not CFS/Linux comparison? | M | research ISA | Yes | Portability | — | Out of scope: HaeunOS guest |
| 24 | Can APPLY brick the guest? | M | gates + rollback ID | Partially | Chaos beyond FI | More FI | Bound failure modes |

## Must-answer in paper (top 5)

1. Two-policy limitation.  
2. RR-only profile collapse.  
3. Autonomy vs opt-in APPLY.  
4. Rollback/safe-point semantics.  
5. Same-host vs third-party reproduction.
