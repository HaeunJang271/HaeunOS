# R2.1 Policy Verification Audit

Read-only audit of the R2.0 implementation before R2.1 changes. Source is authoritative.

## 1. Files and linkage

`tools/build_process_image.py` `KERNEL_SRCS` includes `proc_policy.s` between `proc_sched.s` and `proc_telem.s`. `kernel/scheduler/` is not linked.

## 2. Policy contract (from `proc_policy.s`)

| Item | Actual |
|---|---|
| Entry | `policy_select` |
| Policy ID | Word at `0xA474`; `0` → `policy_rr`, else `r1=0` |
| Input | `r13` = current PCB index |
| Success | `r1=1`, `r6` = slot |
| Failure | `r1=0` |
| Preserved | `r11`, `r13` (documented) |
| Temporaries | Clobbers `r1`–`r8`, `r12` (link), uses `sched_slots` |
| RAM writes | **None** (only `lw` of state / ID) |

`policy_rr`: start at `r13`, scan `+1` with wrap at `sched_slots` (3); accept state READY(1) or RUNNING(2); after 3 misses return none.

Out-of-range `r13` (e.g. 5): first increment wraps to 0; scan still covers 0..2. Not rejected as invalid input.

## 3. Core (`pick_next`)

1. Demote RUNNING → READY + telemetry.
2. Load `curr` → `r13`.
3. `policy_select`.
4. Validate `r6 < sched_slots` and state READY/RUNNING.
5. `telem_select`, RUNNING, `set_curr`, `switch_as`.
6. None → `cur_pcb`, `r1=0` → existing idle paths.

## 4. Callers of `pick_next`

`do_timer`, `resume_or_idle`, `idle_enter`, `idle_trap`.

## 5. Init

`telem_boot_init` clears `0xA464`, `0xA468`, and `0xA474`.

## 6. Baseline provenance

Git `HEAD` `proc_sched.s` predates AROS R1.1 idle/`COUNT+96` work (uncommitted local AROS tree). **No tagged pre-R2 AROS artifact** is available for binary/source diff.

R2.1 equivalence therefore uses preserved R1.1/R1.2/R2.0 deterministic expectations, not `git show HEAD`.

## 7. Discrepancies vs informal docs

None material. Contract matches `docs/r20-scheduler-policy.md`. Register clobber of `r8` during policy is allowed; core rebuilds `r8` after selection.
