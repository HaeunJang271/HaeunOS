# R3.6 Optimizer

Deterministic pairwise selection (RR vs FIFO):

1. Run experiments; classify candidate status.
2. Pareto-mark VALID candidates.
3. Pairwise normalize objectives under profile; weighted score in [0,1].
4. If |score_a−score_b| > min_improve → SELECT winner.
5. Else → EQUIVALENT (retain; no forced switch).

Stability: OPTIMIZE≠APPLY; cooldown field reserved; stale schema rejected on load.
