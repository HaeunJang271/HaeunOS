# R3.6 Pareto Analysis

Axes (when means available):

- avg_ready_wait (minimize)
- jain_fairness (maximize)
- dispatch_count (minimize)

A candidate dominates another if it is no worse on all compared axes and strictly better on ≥1.

Non-dominated candidates are marked `pareto_nondominated=1`.

Final SELECT still requires the active objective profile score gap ≥ `min_improve`;
otherwise EQUIVALENT / NO_DECISION / INCONCLUSIVE.
