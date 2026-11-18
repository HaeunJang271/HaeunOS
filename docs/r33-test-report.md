# R3.3 Test Report

## Suites

- `r33-planner-unit-tests` — idle/CPU keep, safety UNKNOWN path, stability/replay, missing≠zero, shadow no-write
- `r33-planner-guest-tests` — RR/FCFS images, shadow after halt
- Full `make test` includes R1.1–R3.2 regression

## Expected shadow property

Policy word unchanged after `--plan-policy` without `--execute`.

## Replay

Identical planted inputs + identical config → identical decision/target/workload_class.

## Evidence (this tree)

```
make clean && make all   # exit 0
make test                # exit 0 (R1.1–R3.3)
./build/r33-planner-unit-tests.exe          # passed
./build/r33-planner-guest-tests.exe build   # passed
haeun-vm build/r12_yield.hxf --plan-policy  # SHADOW KEEP_CURRENT, policy unchanged
```

Kernel hashes unchanged vs R3.2/R3.3 pre-audit:

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

Example: active READY/RUNNING RR→FIFO interest → `REJECTED_BY_SAFETY` (R3.1 UNKNOWN). Shadow FCFS guest may `RECOMMEND_CHANGE` to RR with PASS preconditions but `txn_committed=false`.
