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
- Guest kernel is **modular monolithic** (not a microkernel claim).
- Process-v1 caps preserved: AS≤3, app≤4KiB, user fault → ZOMBIE ([fault-handling-v1.md](fault-handling-v1.md)).
- Default GUI FB is **320×180** @ `0x00200000` (see [graphics-api.md](graphics-api.md)); shared KBD FIFO remains for legacy demos alongside per-pid `EVENT_POLL`.

## Release pointers

- [getting-started.md](getting-started.md) · [boot-sequence.md](boot-sequence.md) · [limitations.md](limitations.md)
- [platform.md](platform.md) · [software-abi.md](software-abi.md) · [isa-v4.md](isa-v4.md)
- FOSDEM 2027 Kernel Devroom materials: [../research/presentation/fosdem-2027/README.md](../research/presentation/fosdem-2027/README.md)

## Non-goals

Not a bare-metal bootable product OS, not a microkernel product claim, not a production security boundary, and not a full system emulator. See [roadmap.md](roadmap.md) for research direction beyond 1.0.
