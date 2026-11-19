# R4.7 Recommendation Discrimination Validation — Specification

**Track:** Isolated research extension after R4.2.  
**Authority:** R4.7.41 finalized review.  
**Status:** Documentation only (no workloads, tests, or runs in this drop).  
**Does not modify:** R4.2 results, R4.5/R4.6 package, `v4.6.0`, `opt_types.c` baseline profiles, EXCLUDE 12.

Companion index: [`research/experiments/r47-discrimination/README.md`](../research/experiments/r47-discrimination/README.md)  
Cell schema: [`research/experiments/r47-discrimination/schema-r47-cells.schema.json`](../research/experiments/r47-discrimination/schema-r47-cells.schema.json)

## 1. Research question

Under workloads whose measured telemetry can differ between RR (`policy=0`) and preemptive FIFO (`policy=1`, older `ready_seq` first), does `haeun_opt_select` emit decisions that track those measured differences—not only ready-wait collapse on the R4.2 catalog?

## 2. Decision and analysis enums

### 2.1 Recommendation decision (uppercase schema)

| Enum | Meaning |
|------|---------|
| `SELECT_RR` | `haeun_opt_decision` SELECT and `recommended_policy==0` with `score_RR - score_FIFO > min_improve` |
| `SELECT_FIFO` | SELECT and `recommended_policy==1` with `score_FIFO - score_RR > min_improve` |
| `SCORE_EQUIVALENCE` | `EQUIVALENT`: `\|score_RR - score_FIFO\| ≤ min_improve` |
| `NO_DECISION` | Exactly one VALID candidate (`opt_select` NO_DECISION path) |
| `INCONCLUSIVE` | Pair scoring failed or inconclusive without a single-VALID NO_DECISION case |
| `INSUFFICIENT_TELEMETRY` | Schema/analysis label when metrics are missing or candidates are INSUFFICIENT; maps from inconclusive score path |

### 2.2 Measured outcome labels (not optimizer decisions)

| Enum | Meaning |
|------|---------|
| `MEASURED_RR_BETTER` | Predeclared metric favors RR |
| `MEASURED_FIFO_BETTER` | Predeclared metric favors FIFO |
| `MEASURED_EQUIVALENCE` | All declared metrics within predeclared ε |
| `NOT_EVALUATED` | Measured comparison not performed |

**`SCORE_EQUIVALENCE` does not imply `MEASURED_EQUIVALENCE`.** Pairwise min-max scoring can force endpoint scores `{0,1}` on a metric while other measured means stay equal, or cancel after weights.

## 3. Scoring and `min_improve`

Implementation references (unchanged):

- Normalize: `haeun_opt_normalize_pair` in `src/opt_objectives.c` (span `< 1e-12` → both `0.5`; else MIN/MAX endpoint mapping).
- Pair score: `haeun_opt_score_pair` — `score = Σ(w·n) / Σ(w)`.
- Select: `haeun_opt_select` in `src/opt_select.c`; candidates ordered RR then FIFO; `gap = score[0] - score[1]`.
- Default `min_improve = 0.05` (`src/opt_types.c`).

With exactly two candidates, any strict inequality on a metric maps to normalized `{1,0}`. Discrete gaps equal the total weight of discriminating objectives divided by weight sum. Continuous “near-equal raw means ⇒ small score gap” is **false** under this normalizer.

## 4. Predeclared measured tolerances (ε)

Fixed before any new real runs:

| Metric | `MEASURED_EQUIVALENCE` rule |
|--------|-----------------------------|
| `avg_ready_wait` | `\|Δmean\| / max(median, 1) ≤ 0.05` |
| `jain_fairness` | `\|Δ\| ≤ 0.02` |
| `dispatch_count` | `\|Δ\| / max(median, 1) ≤ 0.05` |
| `completed` | `Δ == 0` (integer) |

## 5. Hypotheses

| ID | Statement | Domain |
|----|-----------|--------|
| H1 | Preferable RR metrics beyond `min_improve` ⇒ `SELECT_RR` | Synthetic + real |
| H2 | Preferable FIFO metrics beyond `min_improve` ⇒ `SELECT_FIFO` | Synthetic + real |
| H3 | Score gap within `min_improve` ⇒ `SCORE_EQUIVALENCE` | Synthetic (+ real if observed) |
| H4 | Existing 6×5 baseline remains consistent with R4.2 (30/30 RR) | Regression |
| H5 | Positive **real** discrimination claim requires ≥1 new real cell with **`SELECT_FIFO`**, published raw metrics, and independent measured-outcome columns; `SCORE_EQUIVALENCE` alone never establishes measured performance equivalence |

## 6. Synthetic unit cases (not real evidence)

Profile **latency** unless noted: ready MIN weight `0.75`, dispatch MIN `0.25`.

| ID | Inputs | `min_improve` | Exact scores | Decision |
|----|--------|---------------|--------------|----------|
| S1 | RR ready=40, FIFO=100; dispatch 50/50 | 0.05 | RR 0.875, FIFO 0.125, gap +0.750 | `SELECT_RR` |
| S2 | RR ready=100, FIFO=40; dispatch 50/50 | 0.05 | RR 0.125, FIFO 0.875, gap −0.750 | `SELECT_FIFO` |
| S3 | ready 50/50; dispatch 50/50 | 0.05 | 0.5 / 0.5, gap 0 | `SCORE_EQUIVALENCE` |
| S4 | ready unavailable both | 0.05 | score_valid=0 | `INSUFFICIENT_TELEMETRY` / `INCONCLUSIVE` |
| S5 | only RR candidate VALID | 0.05 | — | `NO_DECISION` |
| S6 | all scored metrics equal | 0.05 | 0.5 / 0.5 | `SCORE_EQUIVALENCE` |
| S7 | same as S1 | **0.74** | gap 0.750 > 0.74 | `SELECT_RR` |
| S8 | same as S1 | **0.76** | gap 0.750 ≤ 0.76 | `SCORE_EQUIVALENCE` |
| S9 | same as S2 | **0.74** | gap −0.750 | `SELECT_FIFO` |

Near-threshold coverage adjusts **`min_improve`**, not tiny raw deltas (see §3).

## 7. Telemetry availability (no invented fields)

**Usable by optimizer `pick()`:** `avg_ready_wait`, `jain_fairness`, `dispatch_count`, `completed`.

**Present in eval/telem aggregates:** per-slot `run_ticks`, `ready_wait_ticks`, `sleep_ticks`, `blocked_ticks`, `dispatch_count`, completion fields, `pid`, `state`; globals `context_switches`, `timer_irqs`; run metrics include `avg_ready_wait`, totals, `slot_run[]`, `slot_ready[]`, `completed` (`include/haeun/telem_inspect.h`, `src/eval_metrics.c`).

**Not exposed for recommendation reporting:** PCB `ready_seq` (+172/+176) is used by FIFO selection in guest code but is **not** in `haeun_telem_slot` / `haeun_telem_read`. Do not require `ready_seq` as a result field. W4 may only describe guest-internal enqueue order.

## 8. Workloads

### 8.1 Baseline (implemented historically)

Six catalog workloads × five frozen profiles — R4.2 reference only. Expect H4 consistency; do not rewrite R4.2 artifacts.

### 8.2 W1–W4 (proposed; **not implemented** in this documentation drop)

Constraints: PCB slots ≤4; step caps similar to catalog (≈3e5–4e5). **No assumed winner.**

**W1 convoy** — Create L then S1,S2; L long CPU, S short; no block; timer preemption as guest; measure at exit via `haeun_eval_metrics_from_vm`.

**W2 equal peers** — Three equal CPU peers, near-simultaneous READY; equal bursts; measure at completion.

**W3 I/O mix** — CPU + blocker; SLEEP/WAIT then wake; measure including sleep/blocked **aggregates** (eval totals; not opt score inputs).

**W4 seq stress** — Scripted ready order A→B→C; A long, B/C short; measure **existing** metrics only (seq not telem-exported).

Event skeleton (all Wi): create → READY insertion → run/preempt → optional block/wake → complete → single post-run metrics snapshot.

## 9. Repetition protocol

- Baseline **n=10** fresh VMs per policy×workload cell (R4.2 style).
- **Extension rule (predeclared):** if for either policy `ready` range/median > 0.25 over the first 10, extend **that cell** once to n=30. No further adaptive fishing.
- Report per policy: mean, median, min, max, variability, score gap, decision.
- No statistical significance unless a separate pre-registered inferential plan exists (default: descriptive only).

## 10. Acceptance criteria

1. Synthetic S1–S9 cover decision branches (when tests are later implemented).
2. Baseline replay consistent with R4.2 or drift explained without rewriting history.
3. Reports keep score vs measured columns separate.
4. Positive real discrimination claim only under refined H5 (§5).
5. Frozen package / `v4.6.0` / baseline profiles / EXCLUDE 12 untouched.

## 11. Unresolved assumptions and limitations

1. Whether future HXF can realize convoy/seq patterns inside AS/PCB/step limits is **unproven**.
2. If jain/dispatch remain policy-identical, `SELECT_FIFO` may never appear — a valid negative result.
3. `INSUFFICIENT_TELEMETRY` is a schema label; map explicitly when emitting from `haeun_opt_decision`.
4. Sensitivity profiles `sens_*` are not defined yet; must be frozen before sensitivity runs.
5. Two policies only; deterministic HXF; descriptive statistics; SHADOW recommendation track separate from APPLY.

## 12. Out of scope for this file drop

No source edits, unit tests, HXF images, experiment execution, builds, staging, commits, pushes, or tags.
