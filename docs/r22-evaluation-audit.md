# R2.2 Evaluation Audit (pre-implementation)

Read-only audit before adding the host-side evaluation framework.
Source is authoritative. No kernel edits are planned for R2.2.

## Verified constraints

| Item | Actual |
|---|---|
| Active scheduler | process-v1 (`kernel/process/`) |
| Core entry | `pick_next` in `proc_sched.s` |
| Policy | `policy_select` / `policy_rr` only; ID `0` at `0xA474` |
| Demo scheduler | `kernel/scheduler/` not linked into process images |
| Telemetry | R1.2 offsets via `haeun_telem_read` (host, read-only) |
| Timer | `COUNT + 96` in `arm_timer` (unchanged for R2.2) |
| R2.1 | Host mirror/trace only; no guest ABI change |

## Pre-R2.2 kernel content hashes (git hash-object)

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `59342fdb8eeeef170469eb338b0cef3c9962d7e2` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `b57636588a35be83e97d731b0ac72ae3b8925600` |

R2.2 completion requires these hashes to remain identical.

## Discrepancies

None material vs R2.0/R2.1 docs. Slot 3 has no AS; FREE slots may hold stale telemetry.

## Workload reuse plan

Existing deterministic userland (no kernel change):

| Category | Image | Notes |
|---|---|---|
| CPU-bound | `r12_run` | spin then EXIT 21 |
| Short / bursty yield | `r12_yield` | 40 YIELDs, EXIT 22 |
| Mixed | `r12_pair` | parent+child YIELD, EXIT 24 |
| Sleep blocking | `r12_sleep` | SLEEP then EXIT 23 (not device I/O) |
| Wait blocking | `r12_wait` | FORK/WAIT, EXIT child code |

Optional R2.2-only userland (`r22_burst`) may be added for explicit burst patterns using existing ECALLs only.

## Unsupported for R2.2

Real device I/O blocking, alternative policies, runtime switching, guest-side tracing ECALLs, wall-clock performance claims.

## Post-R2.2 final audit

After implementation, `git hash-object` on the four kernel files matched the pre-R2.2 hashes above exactly.
No edits to `proc_sched.s`, `proc_policy.s`, `proc_trap.s`, `proc_telem.s`, PCB/telem offsets, or `0xA474`.
No runtime policy switching. Evaluation cost function is host-only.
`make clean` / `make all` / `make test` exit 0 (executed).
