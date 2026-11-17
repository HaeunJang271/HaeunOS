# R3.0 Policy Safety

## Invariants

- Policy layer performs no stores to PCB, `curr`, PTBR, timer, or telemetry.
- Core still validates and commits.
- Invalid policy ID → no-candidate (fail closed).
- No ECALL, no runtime switch, no function pointers, no dynamic modules.
- PCB layout and telemetry offsets unchanged.
- Timer `COUNT+96` unchanged.
- RR scan order unchanged when ID=0.

## Rollover

FCFS uses signed 32-bit modular comparison on `ready_since`.
Valid when pairwise distances among eligible candidates are `< 2^31`.
Intended process-v1 workloads with 3 AS slots stay inside this window.
Beyond that: ordering not guaranteed (documented; no sequence-number ABI change).

## Reused PCBs

`telem_child` zeros +128..+168 and sets a fresh `ready_since`.
Slots do not inherit prior queue age across reuse.

## Configuration safety

`sched_pol_boot` is build-time only. `sched_pol_init` runs once at boot.
Guest apps cannot change `0xA474` through a supported ABI.
