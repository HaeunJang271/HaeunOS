# R3.0 Pre-Implementation Audit

Source-authoritative audit before adding `SCHED_POLICY_FCFS`.
No kernel edits were made while drafting this document.

## 1. Scheduling call graph

```text
do_timer / resume_or_idle / idle_enter / idle_trap
        → pick_next (proc_sched.s)
            1. If curr RUNNING: telem_leave_run; state=READY; telem_ready
            2. r13 ← curr; policy_select
            3. Validate slot < 3 and state READY|RUNNING
            4. telem_select; state=RUNNING; set_curr; switch_as
        → or pn_none → idle paths
```

`policy_select` (proc_policy.s): ID at `0xA474`; `0` → `policy_rr`; else no-candidate.

## 2. RUNNING → READY

In `pick_next`, when current state is RUNNING:

1. `telem_leave_run` accumulates `run_ticks`.
2. State store `READY` (1).
3. `telem_ready` writes `ready_since = COUNT_LO` at PCB+160.

Voluntary leave (`YIELD`/`SLEEP`/`WAIT`/`EXIT` in proc_sys/fork) also calls `telem_leave_run` then sets READY or other states; READY paths call `telem_ready`.

## 3. `ready_since` semantics (critical)

| Event | Effect on +160 |
|---|---|
| Enter READY via `telem_ready` | Set to current `COUNT_LO` |
| Wake SLEEP (`telem_wsleep`) | Accrue sleep_ticks; set `ready_since=now` |
| Wake WAIT (`telem_wwait`) | Accrue blocked_ticks; set `ready_since=now` |
| Fork child (`telem_child`) | Zero +128..+168; set `ready_since=now` |
| `telem_select` on READY | Accrues `ready_wait_ticks`; **does not clear** `ready_since` |
| Timer demotion | Always resets `ready_since` via `telem_ready` |

**Conclusion:** `ready_since` is the time of the **most recent entry into READY**, not a lifetime first-arrival stamp.

It cannot implement classical non-preemptive FCFS “arrival at first ready.”
It **can** implement deterministic **preemptive FIFO / ready-requeue ordering**: after a quantum, the demoted process gets the newest `ready_since` and waits behind older READY peers.

## 4. pick_next and current process

Demotion happens before `policy_select`. At policy time the former RUNNING PCB is READY (when demotion applied). RR may self-select it. FCFS should treat READY|RUNNING like RR for eligibility; order by `ready_since`.

## 5. Timer preemption

`arm_timer` uses `COUNT + 96` (unchanged). Timer remains active under any policy.
Therefore R3.0 FCFS is **not** “traditional non-preemptive FCFS.”
Exact label: **preemptive FIFO ordered by last READY-entry (`ready_since`)**.

## 6. Fork / spawn / reuse

- Child: `telem_child` clears telemetry and sets fresh `ready_since` (no inherited queue age).
- Parent telem not cleared on fork.
- FREE→reuse must go through init paths that set new `ready_since` (child/spawn).

## 7. 32-bit rollover

`ready_since` and `COUNT_LO` are unsigned 32-bit.
Sleep wake already uses signed modular difference with documented `< 2^31` span.

**FCFS plan:** order with `signed(a - b) < 0` ⇒ `a` older than `b`, valid when pairwise distances among eligible candidates are `< 2^31`.
With `SCHED_SLOTS=3` and continuous selection of READY work, intended workloads stay inside this window.
Outside that window: ordering is not guaranteed (documented limitation; no PCB layout change).

## 8. PCB / ABI

Reserved PCB +172..+255 and globals `0xA46C`/`0xA470` exist, but a new sequence number is **not required** if we accept preemptive-FIFO-via-`ready_since`.
R3.0 will **not** change PCB layout, telemetry offsets, or user ABI.

## 9. Implementation decision

| Choice | Decision |
|---|---|
| Proceed? | **Yes**, with precise semantics |
| Variant | Preemptive FIFO by `ready_since` |
| Policy ID | `SCHED_POLICY_FCFS = 1` (`RR = 0` unchanged) |
| Tie-break | Lower PCB slot index |
| Eligibility | READY or RUNNING; slots 0..2 |
| Config | Build-time word `sched_pol_boot` copied to `0xA474` in `telem_boot_init`; default 0 |
| Runtime switch | **None** |
| Sequence number field | **Not introduced** |

## 10. Blockers

None for the preemptive-FIFO interpretation.
Classical non-preemptive FCFS is **out of scope** while timer preemption remains required.
