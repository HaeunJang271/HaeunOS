# R2.0 Scheduler Policy Abstraction

Architectural separation of policy selection from scheduler execution in process-v1.
**Default Round-Robin behavior is unchanged.** Runtime policy switching is not implemented.

## Previous architecture

`pick_next` in `proc_sched.s` interleaved:

1. Demote `RUNNING` → `READY` and telemetry.
2. RR scan (`curr+1` wrap in slots 0..2).
3. Commit (`telem_select`, `RUNNING`, `set_curr`, `switch_as`).

See `docs/r20-scheduler-policy-audit.md`.

## New architecture

```text
do_timer / resume_or_idle / idle_*
        │
        v
   pick_next (core)
        │ demote + telem_leave/ready
        v
   policy_select ──ID≠0──► no-candidate
        │ ID=0
        v
   policy_rr (scan only)
        │
        v
   core validate + telem_select + RUNNING + set_curr + switch_as
```

| Layer | File | Responsibility |
|---|---|---|
| Core | `proc_sched.s` `pick_next` | Demote, invoke policy, validate, mutate state, switch AS, telemetry commit |
| Dispatcher | `proc_policy.s` `policy_select` | Read policy ID; RR or fail closed |
| RR policy | `proc_policy.s` `policy_rr` | Candidate slot or none; **read-only** |

`kernel/scheduler/` remains unlinked.

## Policy interface contract

### Identification

| ID | Name | Storage |
|---:|---|---|
| 0 | `SCHED_POLICY_RR` | Word at `0xA474`, cleared to 0 in `telem_boot_init` |

Only RR is implemented. Invalid IDs return no-candidate (`r1=0`). No user ABI, no ECALL, no runtime switch API.

### Input

- `r13`: current PCB index (`curr`).

### Output

- Success: `r1=1`, `r6=slot` (0..2).
- None / invalid policy: `r1=0`.

### Restrictions (policy)

Must not: write PCB state, write `curr`, change PTBR, restore GPRs, arm/ack timer, or modify telemetry counters.

## RR details (preserved)

- Scan `(curr+1) mod 3`, … until `SCHED_SLOTS` candidates examined.
- Eligible: `READY` (1) or `RUNNING` (2).
- Slot 3 excluded via `sched_slots=3`.
- Self-selection after demotion remains valid.

## Core validation

After policy returns a slot, core requires:

- `slot < sched_slots`
- PCB state is `READY` or `RUNNING`

Then: `telem_select`, store `RUNNING`, `set_curr`, `switch_as`.

No-candidate uses existing R1.1 idle / `idle_dead` paths.

## Telemetry

R1.2 semantics unchanged. Policy selection does not increment counters. Failed selection does not call `telem_select`. R1.3 inspector remains valid (`0xA474` is outside the R1.2 reserved pair `0xA46C`/`0xA470`).

## Timer

`restore_ret` still arms `COUNT + 96`. Extra policy/validate instructions run **before** `arm_timer` on the trap path (IE typically clear). Post-arm / pre-`ERET` margin is unchanged. No silent margin increase.

## Invalid policy behavior

If `0xA474 != 0`, `policy_select` returns none. After a demotion, the system follows the existing no-runnable path (idle or halt). It does not fall through to another policy.

## Deferred (not in R2.0)

Function pointers, dynamic modules, runtime policy switching, priority/MLFQ/CFS/EDF, AI scheduling, reconfiguration transactions.

## Tests

Executed under `make test` (exit 0):

- R1.1 sched/timer, R1.2 telem, R1.3 inspector: pass.
- R2 `rr_yield`: exit 22, `dispatch_count≥40`, `context_switches=0`, `0xA474=0`.
- R2 `rr_pair`: `ctx=61`, `d0=30`, `d1=81` (same relation as R1.2).
- R2 `bad_policy`: poke `0xA474=99` → halt `r1=4`, `context_switches` unchanged.
- R2 `slot3`: sentinel EPC never scheduled.
- R2 `timer_wrap`: ≥3 user timer traps after `0xFFFFFFF0`; `COUNT+96` unchanged.

`ready_wait_ticks` may rise slightly versus pre-R2 on identical user work because demotion→select now includes policy/validate instructions while the PCB is `READY`. Selection order, eligibility, and dispatch/context-switch rules are unchanged.
