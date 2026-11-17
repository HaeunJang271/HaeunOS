# R3.0 FCFS Policy (Preemptive FIFO)

## Why

Provide a second deterministic policy for R2.2 evaluation against RR.
Not runtime reconfiguration. Not classical non-preemptive FCFS.

## Exact variant

**Preemptive FIFO ordered by last READY-entry time (`ready_since` at PCB+160).**

Timer preemption (`COUNT+96`) remains active. On demotion, `telem_ready`
resets `ready_since`, so the preempted process requeues behind older READY peers.

## IDs

| ID | Name |
|---:|---|
| 0 | `SCHED_POLICY_RR` (default) |
| 1 | `SCHED_POLICY_FCFS` |

Stored at `0xA474`. Copied once from build-time `sched_pol_boot` by `sched_pol_init`
during `telem_boot_init`. Fixed for the VM lifetime.

## Eligibility

Slots `0..sched_slots-1` (3). State `READY` or `RUNNING`. Same as RR.

## Ordering

Among eligible slots, select minimum `ready_since` under signed modular order:
`signed(a - b) < 0` ⇒ `a` is older. Tie → lower slot index.

## Interface

Unchanged: `r13=curr`; success `r1=1,r6=slot`; none `r1=0`. Read-only.

## Build / launch

```text
# Default RR images (unchanged)
python tools/build_r12.py build

# FCFS images under build/fcfs/
python tools/build_r30.py build

./build/haeun-eval.exe build short_job 3 --policy rr
./build/haeun-eval.exe build short_job 3 --policy fcfs
```

Default `make` / existing HXF paths remain RR (`sched_pol_boot=0`).
