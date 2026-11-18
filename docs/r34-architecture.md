# R3.4 Architecture

Active runtime policy reconfiguration while eligible processes exist.

Pipeline: Monitor/Plan (R3.3) → Active R3.1 (ready_seq) → R3.2 txn @ between-step → continue guest → host sched trace.

Kernel: READY seq metadata + FIFO compare. Host: snapshot/validation/trace/tests.

Default planner mode remains SHADOW.
