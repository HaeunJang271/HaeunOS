# R3.4 READY Ordering

## Mechanism

64-bit mono `ready_seq` assigned on every READY entry:

| Location | Meaning |
|---|---|
| `0xA46C` / `0xA470` | global counter lo/hi |
| `0xA478` | wrap sticky (set on `2^64` overflow) |
| PCB `+172` / `+176` | per-process seq |

`ready_since` (+160) remains COUNT_LO telemetry.

## FIFO semantics

Preemptive FIFO: eligible READY/RUNNING with **smallest unsigned 64-bit seq** wins; tie → lower slot. Still preemptible by timer.

## Bound

Ordering is proven for validation only while `wrap==0`. Finite counter; not mathematically unbounded.

## Init paths

Boot (`telem_boot_init`), `telem_ready`, `telem_wsleep`, `telem_wwait`, `telem_child`, host `APP_SPAWN`.
