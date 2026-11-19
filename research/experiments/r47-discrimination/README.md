# R4.7 Discrimination Track — Experiment Index

Isolated validation track for scheduler **recommendation discrimination** (RR vs preemptive FIFO).

**Canonical specification:** [`docs/r47-recommendation-discrimination-spec.md`](../../../docs/r47-recommendation-discrimination-spec.md)  
**Final evidence report:** [`docs/r47-final-evidence-report.md`](../../../docs/r47-final-evidence-report.md)  
**Cell JSON Schema:** [`schema-r47-cells.schema.json`](schema-r47-cells.schema.json)

## Integrity

This track **does not** replace or rewrite:

- `research/experiments/r42-profile-verification.json` (30/30 RR OBSERVED)
- R4.5/R4.6 publication package or tag `v4.6.0`
- Baseline objective profiles in `src/opt_types.c`
- EXCLUDE 12 local-only files

R4.2 remains the frozen historical baseline. R47 outputs belong under this directory (and companion docs) only.

## Track status (R4.7.55)

**CLOSED** with a **negative** real-workload result: no completed real cell selected FIFO; W1–W3 recommendations were driven by `avg_ready_wait` as the only differentiating optimizer input. **W4 discontinued** after R4.7.54 (`R4754_W4_NOT_DISTINCT`).

| Item | Status |
|------|--------|
| Specification markdown | Present |
| JSON Schema | Present |
| Synthetic unit tests (Gate A) | **Implemented** — see below |
| W1–W3 HXF workloads + Gates C/D1/D2 | **Completed** (results under `results/`) |
| W4 | **Discontinued** (not distinct under existing telem/optimizer contract) |
| Production source / profile changes | **None** intended by this track’s closeout docs |

## Gate A — Synthetic S1–S9

- Source: [`tests/r47_opt_discrimination_unit_tests.c`](../../../tests/r47_opt_discrimination_unit_tests.c)
- Recorded Gate A campaign report: S1–S9 **passed**
- **No** standalone Gate A results JSON was produced
- This index does **not** claim an independent re-run or reproduction of the unit binary

Synthetic cases exercise decision branches only; they are **not** real-workload evidence. Synthetic `SELECT_FIFO` (S2/S9) does not satisfy H5.

## Gate B — Integration (r12_pair)

| Item | Value |
|------|--------|
| Results | [`results/gate-b/`](results/gate-b/) |
| Tool | `tools/r47_gate_b.py` |
| Reps | **3** per policy (limited) |
| Decision | `SELECT_RR` (balanced) |
| Measured | **`NOT_EVALUATED`** (do not merge with C/D1/D2 measured labels) |

## Gate C — W1

| Item | Value |
|------|--------|
| Results | [`results/gate-c/`](results/gate-c/) |
| Tools | `tests/r47_gate_c_runner.c`, `tools/r47_gate_c.py` |
| Reps | 10 × 5 profiles |
| Outcome | All cells `SELECT_RR`; overall `MEASURED_RR_BETTER` (ready-only differentiation) |

## Gate D1 — W2

| Item | Value |
|------|--------|
| Freeze / amendment | `docs/r47-w2-parameter-freeze.md`, `docs/r47-w2-step-cap-amendment.md` |
| Preserved failure | [`results/gate-d1/failure-300k/`](results/gate-d1/failure-300k/) (300k step-cap) |
| Confirmatory (800k) | [`results/gate-d1/`](results/gate-d1/) (`gate-d1-w2-*`) |
| Outcome | All confirmatory cells `SELECT_RR`; overall `MEASURED_RR_BETTER` |

## Gate D2 — W3

| Item | Value |
|------|--------|
| Freeze | `docs/r47-w3-parameter-freeze.md` |
| Results | [`results/gate-d2/`](results/gate-d2/) |
| Tools | `tests/r47_gate_d2_runner.c`, `tools/r47_gate_d2.py` |
| Outcome | All cells `SELECT_RR`; overall `MEASURED_RR_BETTER`; sleep/blocked analysis-only |

## Decision vs measured outcomes

Recommendation `decision` enums (`SELECT_RR`, `SELECT_FIFO`, `SCORE_EQUIVALENCE`, `NO_DECISION`, `INCONCLUSIVE`, `INSUFFICIENT_TELEMETRY`) are independent of measured labels (`MEASURED_*`).

- `SCORE_EQUIVALENCE` ≠ measured performance equivalence.
- Positive real discrimination (H5) required real `SELECT_FIFO` plus published metrics — **not observed**; see final report.

## Scoring reminder

Two-candidate min-max normalization maps unequal metrics to endpoints `{0,1}`. Score gaps are discrete combinations of objective weights. See spec §3 and synthetic cases S1–S9.

## Telemetry

Only fields available from existing eval/telem/optimizer paths may appear in scored results. PCB `ready_seq` is **not** exported via `haeun_telem_read`.

## Layout

```text
research/experiments/r47-discrimination/
  README.md
  schema-r47-cells.schema.json
  results/
    gate-b/
    gate-c/
    gate-d1/           (+ failure-300k/)
    gate-d2/
```

## Related historical artifacts (read-only)

- `docs/r42-profile-verification.md`
- `research/experiments/r42-performance-results.json`
- `research/paper/r45-main-manuscript.md` (catalog collapse language)
