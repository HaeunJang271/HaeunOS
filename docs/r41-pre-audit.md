# R4.1 Pre-Audit (R4.0 baseline)

Date: 2026-11-18. Method: re-read R4.0 docs + source/tests; no trust of docs alone.

## 1. Already implemented

| Layer | Status |
|-------|--------|
| Guest RR + preemptive FIFO (`0xA474`, ready_seq) | Present in `kernel/process/*` |
| Telemetry + host inspect | R1.2–R1.3 |
| Eval / pval / reconfig / planner / feedback / opt / robust | R2.2–R3.7 host tooling |
| R4.0 integration smoke wrapper | `tests/r40_integration_tests.c` |
| Presets smoke/standard/extended/research | `haeun_rb_preset_*` |

## 2. Actually tested (R4.0)

- `make clean|all|test` exit 0 (**VERIFIED**).
- Smoke robust pipeline reps=3, 6 workloads: EXACT_MATCH / STABLE / CONFIRMED (**OBSERVED**).
- Standard/extended/research campaigns: **NOT TESTED** in R4.0.
- Live reconfig stress: covered by r37 guest, not r40 dry path.

## 3. Supported research claims

- C1–C4, C8–C9: **VERIFIED** (tests).
- C5–C7: **OBSERVED** at smoke / tested conditions.
- Host-assisted framing, SHADOW default, UNKNOWN≠PASS: consistent with code.

## 4. Insufficiently supported

- Inferential policy superiority (C10).
- Generalization beyond fixed HXF catalog.
- SMP / HW atomicity / full VM rollback (correctly not claimed).
- Broader related-work differentiation (R4.0 placeholders only).

## 5. Experiments to repeat / strengthen

1. R4.1 smoke re-verify after docs/tooling changes.
2. Standard campaign (reps=10) if resource estimate permits.
3. Independent recommendation verification record (do not rewrite R3.6 history).
4. Sensitivity under standard preset.

## 6. Unanswered research questions

- How does HaeunOS differ from MAPE-K / adaptive OS literature with verified citations?
- Do recommendations remain CONFIRMED at n=10?
- What is wall-clock cost of standard matrix (~120 baseline runs + sensitivity)?

## 7. What R4.1 must not change

- Kernel process sources (hash lock to R4.0).
- PCB/ECALL/policy IDs/telemetry offsets/scheduler semantics.
- R3.1/R3.2 contracts; historical experiment JSON.
- Workload semantics; no new policies.
