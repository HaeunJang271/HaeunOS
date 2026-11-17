# R3.2 Audit

## Architecture

Host transaction API (`haeun_recfg_*`) + R3.1 validation + between-step safe point + single-word policy commit + policy-ID rollback + test-only FI + structured logs/reports + CLI.

## Kernel baseline

R3.2 does **not** modify `proc_sched.s` / `proc_policy.s` / `proc_trap.s` / PCB layout / telemetry offsets / timer `COUNT+96` / default boot RR.

Post-implementation `git hash-object` (unchanged vs pre-audit):

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

## Build evidence

```
make clean && make all   # exit 0
make test                # exit 0 (includes R1.1–R3.2)
./build/r32-reconfig-unit-tests.exe
./build/r32-reconfig-guest-tests.exe build
haeun-vm build/fcfs/r12_yield.hxf --reconfigure-policy fcfs --to rr  # COMMITTED
haeun-vm build/r12_yield.hxf --reconfigure-policy rr --to fcfs       # COMMITTED (vacuous)
```

## Claims vs non-claims

| Claim | Status |
|---|---|
| Runtime switch in one VM lifetime | Yes (host API after steps) |
| Both directions tested | Yes (FIFO→RR success; RR→FIFO success only if vacuous else reject) |
| UNKNOWN≠PASS | Yes |
| Serialized commit | Yes |
| Hardware atomicity / SMP | **Not claimed** |
| Full VM rollback | **Not claimed** |
| Formal verification | **Not claimed** |
| Unrestricted guest ECALL switch | **Not provided** |

## CLI

```
haeun-vm IMAGE --reconfigure-policy fcfs --to rr
haeun-vm IMAGE --reconfigure-policy rr --to fcfs
```

Requires explicit source and target. Nonzero exit on reject/fail. Does not silently fall back. Does not patch boot policy and call it runtime.

## Concurrency

Single-threaded interpreter ownership. One transaction at a time. No multi-core claim.

## Tests

- `r32-reconfig-unit-tests`
- `r32-reconfig-guest-tests`
- Full `make test` regression including R1.1–R3.1

## Timestamp blocker

Without a new bounded epoch/sequence in PCB ABI, RR→FIFO with eligible READY/RUNNING remains rejected when R3.1 returns UNKNOWN. This is intentional.
