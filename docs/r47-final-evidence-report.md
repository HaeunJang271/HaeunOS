# R4.7 Final Evidence Report — Recommendation Discrimination Validation

**Track:** R47 Recommendation Discrimination Validation  
**Authority:** R4.7.55 documentation consolidation  
**Predecessor audits:** R4.7.54 (`R4754_W4_NOT_DISTINCT`)  
**Mode:** Documentation only; no re-runs, no source/result mutation

**Closing conclusion (negative, preserved):** No completed real workload selected FIFO. Gates C/D1/D2 repeatedly selected RR because `avg_ready_wait` was the only differentiating optimizer input. W4 is discontinued.

---

## 1. Research question and scope

**Question** (from `docs/r47-recommendation-discrimination-spec.md`): Under workloads whose measured telemetry can differ between RR (`policy=0`) and preemptive FIFO (`policy=1`), does `haeun_opt_select` emit decisions that track those differences—not only ready-wait collapse on the R4.2 catalog?

**In scope:** Synthetic Gate A; Gate B r12_pair isolation; real workloads W1–W3 (Gates C/D1/D2); freeze/amendment docs; cell/raw JSON; schema.

**Out of scope:** R4.2 rewrite; optimizer profile/weight changes; new telemetry fields; APPLY mode; statistical significance claims; general scheduling-policy superiority claims; W4 execution.

**Isolation:** Results under `research/experiments/r47-discrimination/`; production catalog and `v4.6.0` untouched.

---

## 2. Gate A — Synthetic S1–S9 (not real workload evidence)

| Item | Status |
|------|--------|
| Source | `tests/r47_opt_discrimination_unit_tests.c` |
| Standalone results JSON | **None produced** |
| This consolidation | Does **not** re-execute or independently reproduce the binary |

Recorded Gate A campaign completion reported S1–S9 PASS against the branch table in the specification (§6). Synthetic cases only:

| ID | Expected analysis decision |
|----|----------------------------|
| S1, S7 | `SELECT_RR` |
| S2, S9 | `SELECT_FIFO` |
| S3, S6, S8 | `SCORE_EQUIVALENCE` |
| S4 | `INSUFFICIENT_TELEMETRY` / `INCONCLUSIVE` |
| S5 | `NO_DECISION` |

Synthetic `SELECT_FIFO` (S2/S9) is **not** a real-workload H5 claim.

---

## 3. Gate B — r12_pair integration (limited repetitions)

| Item | Value |
|------|--------|
| Artifacts | `results/gate-b/gate-b-r12-pair-{raw,cell}.json` |
| Runner/tools | `tools/r47_gate_b.py` |
| Profile | `balanced` only |
| Reps per policy | **3** (not the n=10 workload protocol) |
| RR success/fail | 3 / 0 |
| FIFO success/fail | 3 / 0 |
| Decision | `SELECT_RR` (scores 0.7 / 0.3, gap 0.4) |
| Measured outcomes | **All `NOT_EVALUATED`** (overall `NOT_EVALUATED`) |

**Metrics (denominator n=3 successful runs each):**

| Policy | ready_mean | jain_mean | dispatch_mean | completed_mean |
|--------|------------|-----------|---------------|----------------|
| RR | 15439.0 | 0.913645 | 111.0 | 2.0 |
| FIFO | 18778.0 | 0.913645 | 111.0 | 2.0 |

Gate B demonstrates isolated runner wiring. Its `NOT_EVALUATED` measured column must **not** be merged with Gates C/D1/D2 measured labels.

---

## 4. Gate C — W1 (real workload)

| Item | Value |
|------|--------|
| Workload | W1-B′ yield-gated convoy (`userland/r47_w1_convoy.s`) |
| Artifacts | `results/gate-c/gate-c-w1-*-{raw,cell}.json` |
| Runner/tools | `tests/r47_gate_c_runner.c`, `tools/r47_gate_c.py` |
| Reps | **10** per policy × 5 profiles |
| Totals | RR 50/50/0; FIFO 50/50/0 (requested/success/fail) |

**Shared metrics (identical across all five profile cells; stddev 0):**

| Policy | ready_mean | jain_mean | dispatch_mean | completed_mean |
|--------|------------|-----------|---------------|----------------|
| RR | 65771.666667 | 0.468032 | 537.0 | 3.0 |
| FIFO | 78741.0 | 0.468032 | 537.0 | 3.0 |

**Measured (every profile):** ready `MEASURED_RR_BETTER`; jain/dispatch/completed `MEASURED_EQUIVALENCE`; overall `MEASURED_RR_BETTER`.

---

## 5. Gate D1 — W2 (real workload)

Freeze: `docs/r47-w2-parameter-freeze.md`. Amendment: `docs/r47-w2-step-cap-amendment.md`.

### 5.1 Preserved 300k failure (not confirmatory)

| Item | Value |
|------|--------|
| Path | `results/gate-d1/failure-300k/` |
| Caps | Original freeze **300,000** |
| RR/FIFO | 1 rep each; both `fail_reason=step cap exceeded` |
| Raw opt | `INCONCLUSIVE` |
| Cell decision | `INSUFFICIENT_TELEMETRY`; scores null; measured `NOT_EVALUATED` |

### 5.2 Approved 800k confirmatory campaign

| Item | Value |
|------|--------|
| Step cap | **800,000** (user-approved before resume) |
| Artifacts | `results/gate-d1/gate-d1-w2-*-{raw,cell}.json` |
| Totals | RR 50/50/0; FIFO 50/50/0 |

**Shared metrics (all five profiles; stddev 0):**

| Policy | ready_mean | jain_mean | dispatch_mean | completed_mean |
|--------|------------|-----------|---------------|----------------|
| RR | 367021.666667 | 0.985636 | 1287.0 | 3.0 |
| FIFO | 461741.0 | 0.985636 | 1287.0 | 3.0 |

**Measured (every profile):** overall `MEASURED_RR_BETTER` (ready only).

---

## 6. Gate D2 — W3 (real workload)

Freeze: `docs/r47-w3-parameter-freeze.md` (step_cap **800,000** fixed).

| Item | Value |
|------|--------|
| Artifacts | `results/gate-d2/gate-d2-w3-*-{raw,cell}.json` |
| Runner/tools | `tests/r47_gate_d2_runner.c`, `tools/r47_gate_d2.py` |
| Totals | RR 50/50/0; FIFO 50/50/0 |

**Shared optimizer metrics (all five profiles; stddev 0):**

| Policy | ready_mean | jain_mean | dispatch_mean | completed_mean |
|--------|------------|-----------|---------------|----------------|
| RR | 76037.666667 | 0.636445 | 432.0 | 3.0 |
| FIFO | 90984.333333 | 0.636445 | 432.0 | 3.0 |

**Analysis-only (raw; not cell-scored):** `total_sleep` RR=358, FIFO=417; `total_blocked`=0 both. Overall measured: `MEASURED_RR_BETTER`.

---

## 7–9. Scores, decisions, measured classifications

`min_improve=0.05` throughout confirmatory cells.

| Profile | score_RR | score_FIFO | gap | decision (C/D1/D2) |
|---------|----------|------------|-----|---------------------|
| balanced | 0.7 | 0.3 | 0.4 | `SELECT_RR` |
| fairness | 0.65 | 0.35 | 0.3 | `SELECT_RR` |
| latency | 0.875 | 0.125 | 0.75 | `SELECT_RR` |
| low_overhead | 0.65 | 0.35 | 0.3 | `SELECT_RR` |
| throughput | 0.7 | 0.3 | 0.4 | `SELECT_RR` |

Gate B balanced matches the same score row but measured=`NOT_EVALUATED`.

C/D1/D2 measured pattern: ready `MEASURED_RR_BETTER`; jain/dispatch/completed `MEASURED_EQUIVALENCE`; overall `MEASURED_RR_BETTER`.

**Real cells with `SELECT_FIFO`:** none.  
**Real cells with `MEASURED_FIFO_BETTER`:** none.

---

## 10. Repeated `avg_ready_wait`-only discrimination

Across Gates C, D1, and D2:

1. Jain, dispatch, and completed means are **policy-identical**.
2. Only `avg_ready_wait` differs (RR lower).
3. Two-candidate min-max maps that single inequality to endpoints `{1,0}` for the ready objective; tied objectives contribute `0.5`.
4. All five profiles therefore `SELECT_RR` with discrete gaps equal to ready’s relative weight share—not because five independent empirical axes favored RR.

Implementation correctness (optimizer tracks ready differences) ≠ empirical multi-axis discrimination capability.

---

## 11. H5 negative result

Spec H5: a positive **real** discrimination claim requires ≥1 new real cell with `SELECT_FIFO`, published raw metrics, and independent measured-outcome columns.

**Outcome:** H5 **not met**. The track closes with a documented **negative** result: real workloads did not produce FIFO recommendation or FIFO-favoring measured overall labels under the frozen optimizer contract.

---

## 12. W3 SLEEP / WAIT telemetry interpretation

- `total_sleep ≠ 0`: child B executed `sys_sleep(64)`; sleep ticks accumulated via existing telem/eval paths.
- `total_blocked = 0` despite parent `WAIT×2`: L waits after its CPU burst; children are typically already zombies, so WAIT often reaps **without** entering the blocking/`telem_block` path. Zero blocked ticks means blocking was not observed, not that WAIT syscalls were absent.
- Sleep/blocked are **analysis-only** (W3 freeze); they do not enter `haeun_opt_score_pair`.
- Recommendation validity for W3 rests on ready-wait differences, not on blocked-tick discrimination.

---

## 13. W4 discontinuation (R4.7.54)

R4.7.54 verdict: **`R4754_W4_NOT_DISTINCT`**.

W4 (“seq stress”) would measure only existing optimizer metrics while `ready_seq` remains non-exported. Under that contract it would reproduce another ready-wait comparison, not a genuinely new discrimination axis. **W4 is discontinued; not designed further, not frozen, not executed.**

---

## 14. Limitations and reproducibility

| Limitation | Note |
|------------|------|
| Descriptive only | No inferential statistics; no general policy-superiority claim |
| Deterministic cells | Confirmatory ready stddev 0 within each policy |
| Gate B n=3 | Integration scale; measured `NOT_EVALUATED` |
| Gate A | No results JSON; not re-verified here |
| Telemetry | `ready_seq` not in telem; sleep/blocked absent from B/C/D1 raw schema |
| README lag | Index README corrected in R4.7.55; historical spec drop text may still say “proposed” |
| Isolation | Untracked R47 surface may remain local; does not alter `v4.6.0` |

**Reproducibility status:** Raw and cell JSON for Gates B–D2 are preserved under `research/experiments/r47-discrimination/results/`. Schema: `schema-r47-cells.schema.json`. Freeze/amendment docs list parameters. This report does not re-run experiments.

---

## 15. Artifact index

| Kind | Path |
|------|------|
| Spec | `docs/r47-recommendation-discrimination-spec.md` |
| W2 freeze / amendment | `docs/r47-w2-parameter-freeze.md`, `docs/r47-w2-step-cap-amendment.md` |
| W3 freeze | `docs/r47-w3-parameter-freeze.md` |
| Index | `research/experiments/r47-discrimination/README.md` |
| Schema | `research/experiments/r47-discrimination/schema-r47-cells.schema.json` |
| Gate A tests | `tests/r47_opt_discrimination_unit_tests.c` |
| Results | `research/experiments/r47-discrimination/results/gate-{b,c,d1,d2}/` |

**Track status: CLOSED** with negative real-workload discrimination result; W4 discontinued.
