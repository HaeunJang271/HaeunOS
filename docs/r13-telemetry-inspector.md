# R1.3 Read-only Telemetry Inspector

Status: host-side diagnostic for process-v1 R1.2 counters. Observes the kernel; does not control it.

## Architecture

The inspector lives entirely on the host:

| Piece | Path |
|---|---|
| API | `include/haeun/telem_inspect.h` |
| Implementation | `src/telem_inspect.c` |
| CLI | `haeun-vm --telem` / `--telem-json` (after the guest run) |
| Tests | `tests/r13_telem_inspect_tests.c` |

It reads `vm->ram` with little-endian 32-bit loads. It never writes guest RAM, never touches timer MMIO, never steps the guest, and never issues ECALLs.

## Memory map (unchanged from R1.2)

PCB base `0xA000`, stride `0x100`, NPROC `4`.

Per-PCB telemetry offsets: `+128`…`+168` (`run_ticks` … `blocked_since`). `+172`…`+255` reserved.

Globals: `0xA460` `IDLE_FLAG`, `0xA464` `timer_irqs`, `0xA468` `context_switches`, `0xA46C`/`0xA470` reserved.

Current index: `0xA400`.

## Counter semantics

Identical to R1.2 / `docs/process-v1.md`. Highlights:

- `run_ticks` includes kernel handler time while the PCB remains `RUNNING`.
- `dispatch_count` includes self-selection; `context_switches` counts index changes only.
- `timer_irqs` counts guest `do_timer` / `idle_trap` entries.
- `complete_lo` is meaningful only when `complete_valid != 0`.
- Values are raw unsigned 32-bit (wrap at `2^32`). The inspector does not invent elapsed durations from timestamps.

## State decoding

| Value | Name |
|---:|---|
| 0 | FREE |
| 1 | READY |
| 2 | RUNNING |
| 3 | SLEEP |
| 4 | WAIT |
| 5 | ZOMBIE |

Slot 3 is printed with `(no AS)` because process-v1 has only three address spaces (`sched_slots = 3`).

## Consistency notes

Notes are labeled `info`, `expected`, or `warn`:

- Slot 3 lacking an AS is **info** (expected architecture).
- FREE slots with non-zero counters are **expected** stale telemetry, not errors.
- `context_switches` greater than the sum of `dispatch_count` is a **warn**.
- `complete_valid` on a live non-ZOMBIE/non-FREE state is a **warn**.
- Zero counters alone never produce a warning.

## Read-only guarantees

`haeun_telem_read` and the print helpers take `const haeun_vm *` (or only the filled report). Tests snapshot the PCB page (`0xA000`…`0xAFFF`) and compare timer CSRs / EPC / PTBR / user SP before and after inspection.

## Example output

```text
HaeunOS Process Telemetry
=========================

Global
------
curr_index       : 0
IDLE_FLAG        : 0
timer_irqs       : 7
context_switches : 0

PCB Telemetry
-------------

Slot 0
  PID             : 1
  State           : ZOMBIE (5)
  Run ticks       : 1954
  ...
  Completion      : <complete_lo when valid>

Slot 3 (no AS)
  ...
```

Numbers come from guest RAM at inspection time; they are never hardcoded.

`--telem-json` emits one JSON object with globals and per-slot fields for tooling.

## Raw vs interpreted

| Raw | Interpreted |
|---|---|
| Numeric `state` | Named state string |
| `complete_lo` always present in JSON | Text report shows `N/A` unless `complete_valid` |
| Slot 3 counters | Annotated as no address space |
| Stale FREE counters | Expected note, not an inconsistency |

## Limitations

- Does not reconstruct live READY/SLEEP intervals from `*_since` timestamps (wrap risk).
- Reserved globals `0xA46C` / `0xA470` are not displayed.
- Does not provide autonomous reconfiguration, policy switching, or scheduler control.
- Requires the process-v1 PCB page to be present in `vm->ram`.

## Test methodology

1. Plant known words in RAM; verify address math, unsigned printing, completion `N/A`, FREE stale **expected** note, slot 3 `as_slot_valid == 0`, and PCB-page immutability.
2. Run `r12_run.hxf` to halt; compare inspector fields to direct `rd32`; confirm PCB page and timer/EPC/PTBR/SP unchanged after `haeun_telem_read` and print.
3. Existing R1.1 / R1.2 suites must still pass unchanged.
