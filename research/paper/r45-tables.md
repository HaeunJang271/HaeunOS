# R4.5 Publication Tables

**Baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Sources only:** existing R4.2–R4.4 artifacts. No derived missing cells.

## T1. Policy comparison (evaluated surface)

| Policy ID | Name | Ordering basis | Notes |
|-----------|------|----------------|-------|
| 0 | RR | Round-robin among READY/RUNNING | Default boot policy in many images |
| 1 | Preemptive FIFO | Unsigned 64-bit `ready_seq` (+172/+176); tie → lower slot | Not non-preemptive FCFS |

**Not evaluated:** additional policies / Linux CFS / lottery as runnable guest IDs.

## T2. Descriptive catalog `ready_wait` (R4.2)

Source: `docs/r42-performance-analysis.md` ← `r42-performance-results.json` / `r42-statistics.json`.  
n=10 per cell; often deterministic. Units: guest ticks. **Not inferential.**

| Workload | RR mean | FIFO mean | Jain RR | Jain FIFO |
|----------|---------|-----------|---------|-----------|
| cpu_bound | 749 | 812 | 1 | 1 |
| short_job | 4920 | 5280 | 1 | 1 |
| mixed | 15439 | 18778 | 0.913645 | 0.913645 |
| bursty | 4600 | 4960 | 1 | 1 |
| block_sleep | 159 | 168 | 1 | 1 |
| block_wait | 216 | 254 | 0.506954 | 0.506954 |

Campaign: planned 120, success 120, fail 0.

## T3. Experimental campaign coverage

| Campaign | Planned | Executed | Independent 3rd party |
|----------|---------|----------|------------------------|
| R4.1 standard 6×2×10 | YES | YES (suites) | NOT TESTED |
| R4.2 performance 120 | YES | YES | NOT TESTED |
| R4.2 profiles 30 | YES | YES (all SELECT RR) | NOT TESTED |
| R4.3 apply RR↔FIFO | YES | YES; 61/61 mirror | NOT TESTED |
| R4.3 reject/rollback | YES | YES | NOT TESTED |
| 30-cell live APPLY | mapped | **NOT** (`NOT_TESTED_per_cell`) | — |
| R4.3.2 worktree rebuild | YES | chain exit 0 | NOT TESTED |

## T4. Safety invariant matrix

| Principle | Grade | One-line |
|-----------|-------|----------|
| SHADOW default | VERIFIED | Apply is opt-in |
| UNKNOWN≠PASS | VERIFIED | Prepare rejects |
| Prepare ≠ commit | VERIFIED | Separate APIs |
| Opt ≠ authorize | VERIFIED | Must use reconfig |
| Rollback = policy ID | OBSERVED | Not full VM |
| Between-step safe point | PARTIAL | Interpreter convention |
| SMP / HW atomicity | NOT CLAIMED | Explicit non-goal |

## T5. Claim / evidence class summary

| Class | Examples | Rule |
|-------|----------|------|
| A | pipeline; gates; recorded 61/61 | Claimable |
| B | autonomy wording; descriptive RR wait; same-host repro | Qualify |
| C | significance; fairness↑; 3rd party success; full rollback; SMP | Forbidden |

Full rows: `docs/r44-claim-evidence-matrix.md`.

## T6. Limitations and threats

| Threat | Mitigation in paper |
|--------|---------------------|
| Two policies only | State minimal surface |
| 30/30 RR recommendations | Report collapse |
| Deterministic HXF | No p-values |
| Mirror ≠ omniscient guest proof | Scope 61/61 recordings |
| Same-host worktree | Third-party NOT TESTED |
| Policy-ID rollback | Do not say crash restore |
