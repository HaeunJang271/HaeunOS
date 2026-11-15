# HAEUN-OS architecture overview (Release 1.0)

## Goals and invariants

HAEUN-OS targets x86-64 Windows and Linux hosts while defining a separate 32-bit guest. The guest ISA is HAEUN-32; host execution is through the native C17 virtual machine. Host pointers, host endianness, and host instructions are never guest architectural state.

Initial machine profile: one virtual CPU, 256 MiB physical RAM, 32-bit virtual addresses, 4 KiB pages, little-endian. Scheduling is cooperative (`SYS_YIELD`) and timer-preemptive (IRQC source 0).

## Layering

1. Host process: C17 VM and device models (`src/`).
2. Virtual board: RAM, Boot ROM, IRQC, timer, UART, block, FB, KBD, mouse, NIC.
3. Firmware (`firmware/boot.s`): validates disk HXF, builds HBI, transfers to entry.
4. Guest kernel: foundation or process-v1 (`kernel/process/*`).
5. Host assists: ECALL mux (HFS/GUI/sockets/apps) + fault_io (M17).
6. Userland: demos and M14 apps (`userland/`).

Each boundary uses versioned formats or explicit register contracts.

## Decisions

- Fixed 32-bit instruction words; arithmetic wraps modulo 2^32.
- Interpreter implements ISA v4 privilege/traps and MMU v1 + TLB/`SFENCE`.
- Release 1.0 preserves M14–M17 contracts: AS≤3, app≤4KiB, FB 64×32, shared KBD FIFO, user fault → ZOMBIE.

## Release pointers

- [getting-started.md](getting-started.md) · [boot-sequence.md](boot-sequence.md) · [limitations.md](limitations.md)
- [platform.md](platform.md) · [software-abi.md](software-abi.md) · [isa-v4.md](isa-v4.md)

## Non-goals

Not a bare-metal bootable product OS, production security boundary, or full system emulator. See [roadmap.md](roadmap.md) for research direction beyond 1.0.
