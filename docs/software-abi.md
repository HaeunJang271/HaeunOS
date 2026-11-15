# Kernel, calling convention, executable and boot contracts

## Kernel structure

The target kernel is a modular monolith: one privileged address space with internal module interfaces for architecture, memory management, scheduler, IPC, VFS, and drivers. Initial boot is single CPU. The foundation in [kernel-foundation.md](kernel-foundation.md) boots through the firmware path with logging, panic/assert, and real PMM/heap. The scheduler demo in [kernel-v1.md](kernel-v1.md) provides an `ECALL` syscall trap (`SYS_EXIT`, `SYS_PUTC`, `SYS_YIELD`) with timer preemption.

Kernel C is C17 with fixed-width integer types at hardware and binary boundaries. HAEUN Assembly is required for reset entry, exception entry/return, context switch, and atomic/interrupt-sensitive primitives once those ISA facilities are specified. Drivers depend on kernel services rather than host APIs. VM host-side code is C17 and must not expose host pointers to guest code.

## Calling convention v1 (provisional)

- `r0`: constant zero.
- `r1`: return value and scratch; caller-saved.
- `r2–r7`: argument registers; caller-saved.
- `r8–r11`: callee-saved.
- `r12`: global pointer, reserved by ABI.
- `r13`: stack pointer, 16-byte aligned at public call boundaries; stack grows down.
- `r14`: inclusive lower bound of the current stack reservation in the C subset; callee-saved. It is task-specific and must be installed by the boot/runtime context. The current VM CLI initializes it to `0x0FF00000`.
- `r15`: link/return-address register for JAL/JALR, caller-saved.
- Integer scalar arguments/results are 32-bit. 64-bit values use consecutive even/odd register pairs when available, otherwise stack slots. Aggregates are passed by address. Caller allocates outgoing stack arguments; stack slots are 4-byte aligned. No red zone.

The tiny compiler tests this subset with six register arguments, stack-passed arguments, stack-homed excess parameters/locals, temporary spills, caller-register preservation, helper prologues/epilogues, recursion, and stack-overflow stopping. Before allocating a frame or outgoing stack arguments, generated code checks the prospective stack pointer against `r14`; a violation branches to an `ECALL` stop. See the [compiler subset ABI](compiler-subset.md) for frame offsets and current limits. General VM-enforced stack bounds and broader kernel integration remain unimplemented, so the full convention is still provisional.

## HAEUN executable format (HXF v1)

All fields are little-endian. Header is exactly 64 bytes; no native structs may be serialized directly. Offsets:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic bytes `H X F 1` |
| 4 | 2 | header_size = 64 |
| 6 | 2 | format_version = 1 |
| 8 | 4 | guest ISA version, 1..4 |
| 12 | 4 | flags; v1 requires 0 |
| 16 | 4 | entry virtual address |
| 20 | 4 | segment table file offset |
| 24 | 2 | segment count, 1..16 |
| 26 | 2 | segment entry size = 32 |
| 28 | 4 | total file size in bytes |
| 32 | 32 | reserved, must be zero |

Each 32-byte segment entry: `type:u32` (1=LOAD), `flags:u32` (bit 0 read, bit 1 write, bit 2 execute; no other bits), `file_offset:u32`, `virtual_address:u32`, `file_size:u32`, `memory_size:u32`, `alignment:u32`, `reserved:u32` (zero). Segment table and file ranges must be in bounds without arithmetic overflow; `file_size <= memory_size`; alignment must be a power of two and at least 4; file and virtual offsets must satisfy alignment; LOAD segments may not overlap in virtual memory or file payload; payloads may not overlap the header or segment table; entry must be 4-byte aligned and inside an executable LOAD segment. Segment virtual memory must not exceed the user range. Zero-fill `memory_size-file_size` bytes. Unknown types/flags, nonzero reserved fields, bad versions, duplicate/overlapping ranges, or invalid entry reject the whole image before modifying guest memory. No checksum/signature is present in v1; image authenticity is out of scope for the prototype.

The initial VM loader uses a flat RAM profile: every segment virtual address must also fit in the supplied guest RAM, because paging and virtual-to-physical translation are not implemented. This restriction belongs to the prototype loader, not the HXF format.

The provided writer emits the table immediately after the header, preserves caller segment order, aligns each payload offset upward to the segment alignment, and writes zero padding. It first computes the complete size; insufficient output capacity reports the required size and leaves the output buffer untouched. Source segment buffers must be disjoint from the output buffer. The loader requires the input image buffer and guest RAM to be disjoint.

## Bootloader responsibilities

The eventual ROM boot stage validates HXF before loading, zeros BSS tails, ensures segment permissions, constructs initial page tables and the [HBI v1 boot-information record](bootinfo-v1.md), initializes device state, and transfers control to entry with interrupts disabled. Initial register handoff: `r2` points to boot info; `r3` contains its byte size; `r13` is 16-byte aligned stack top; `r14` is the inclusive lower bound of the initial stack reservation; remaining registers unspecified. Bootloader recovery and malformed-image behavior: print a fixed UART diagnostic and halt. Firmware disk boot implements this handoff ([boot-disk-v1.md](boot-disk-v1.md)).
