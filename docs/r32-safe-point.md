# R3.2 Safe Point

## Exact safe point

Host reconfiguration APIs run **between** `haeun_vm_step` calls (including after halt).

Evidence: the VM is a single-threaded host interpreter. Guest instructions, traps, and timer delivery execute only inside a step. No second host thread drives the same VM in the production CLI.

## Why scheduler execution cannot race the commit

`policy_select` / `pick_next` / `set_curr` / `switch_as` execute only when the guest kernel path runs, which requires a step. A host write to `0xA474` performed between steps cannot interleave mid-selection.

## Timer interrupts

Timer compare/count advances and trap entry occur inside a step. At the between-step boundary, no timer trap is mid-flight in guest code.

## Failure / timeout

R3.2 does not wait for a future safe point. The caller is already at a safe point or the request is rejected. There is no interrupt-disable critical section and no unbounded quiesce loop.

## Claim accuracy

This is **serialized policy commit** under a single-threaded step model. It is **not** hardware atomicity, SMP safety, or multi-core scheduler locking.
