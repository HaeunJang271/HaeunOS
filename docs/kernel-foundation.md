# HAEUN-OS kernel foundation

Status: **guest C17 subset + HAEUN Assembly modular monolith, bootloader path tested**.

## Architecture

One privileged address space (modular monolith). Module roots:

| Path | Role |
|---|---|
| `kernel/core` | Entry orchestration, status codes, logging, panic, assertions |
| `kernel/arch` | Reset handoff, stack check, CPU CSR bring-up |
| `kernel/interrupt` | Exception vector install and early trap stub |
| `kernel/mm` | PMM, identity VMM, bump heap (no fake allocators) |
| `kernel/drivers` | UART driver |
| `kernel/syscall` | Deferred interface (not in foundation success path) |
| `kernel/process` | Deferred interface |
| `kernel/scheduler` | Existing timer-preemptive demo (`sched.s`) |
| `kernel/vfs` | Deferred interface |

Guest code is compiled/assembled by the HAEUN toolchain and executed only by the VM. Host shortcuts that poke kernel state are not used for the foundation boot path.

## Status codes

Every init function returns an explicit `int` status (`0` = success). Non-zero aborts the sequence via `kpanic`; the kernel must not print the foundation-ready banner after a failed required step.

| Code | Name | Meaning |
|---:|---|---|
| 0 | `K_OK` | Success |
| 1 | `K_ERR_ARG` | Invalid argument |
| 2 | `K_ERR_STATE` | Bad boot/CPU state |
| 3 | `K_ERR_IO` | Device failure |
| 4 | `K_ERR_NOMEM` | Allocator exhausted |
| 5 | `K_ERR_ASSERT` | Assertion failure |
| 6 | `K_ERR_DEFERRED` | Module reserved, not started |

## Initialization sequence

1. Bootloader loads HXF, writes HBI, jumps to `_start` (`kernel/arch/entry.s`).
2. Verify `r13`/`r14` stack bounds against HBI; align `r13`.
3. `cpu_init` — supervisor status baseline (ISA 4).
4. `trap_init` — install `trapvec` early stub.
5. `uart_init` — enable UART TX path.
6. `klog_init` — logging ready (uses UART).
7. `pmm_init` / `vmm_init` / `heap_init` — real page freelist + identity VMM + bump heap.
8. Emit kernel initialization log; `HALT` with `r1=0` on success.

A failed step prints a panic line and halts with nonzero `r1`.

## Memory interfaces

- **PMM**: intrusive freelist over a fixed physical arena (`pmm_base`..`pmm_end` = `0x00400000`..`0x00800000`); `pmm_alloc` / `pmm_free` move real pages. Metadata at `pmm_meta` (`0x003FF000`).
- **VMM**: identity map only; `vmm_map(va,pa)` accepts `va==pa` inside RAM and returns `va`.
- **Heap**: bump allocator backed by `pmm_alloc`; returns `0` on exhaustion.

## Boot path

```text
haeun-vm --firmware boot.hfw --disk kernel.raw
```

Firmware accepts ISA 1..4 single-LOAD HXF images (`file_size <= 0x8000`). The foundation image is ISA 4.
