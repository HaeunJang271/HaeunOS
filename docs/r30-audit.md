# R3.0 Final Audit

## Kernel modifications

| File | Change |
|---|---|
| `kernel/process/proc_policy.s` | `policy_fcfs`; dispatch ID=1; `sched_pol_boot` / `sched_pol_init` |
| `kernel/process/proc_telem.s` | `telem_boot_init` calls `sched_pol_init` instead of forcing 0 |

Unchanged: `proc_sched.s` pick_next structure, `proc_trap.s`, timer `COUNT+96`, PCB/telem offsets, RR algorithm when ID=0.

## Host / tools

- `tools/build_process_image.py`: optional `policy_id` patches `sched_pol_boot`.
- `tools/build_r30.py`: emits `build/fcfs/*.hxf`.
- `src/policy_verify.c`: FCFS host mirror.
- Eval runner/CLI: `--policy rr|fcfs`, policy fields in reports/compare.

## Confirmations

- RR default (`sched_pol_boot=0`).
- FCFS selected before first schedule via boot copy to `0xA474`.
- No runtime switching / ECALL / AI / dynamic modules.
- Policy read-only (guest test snapshot).
- R1.1–R2.2 regressions included in `make test`.

## Claims not made

- Classical non-preemptive FCFS.
- FCFS superiority.
- Formal verification.
- Rollover safety beyond `< 2^31` pairwise window.
