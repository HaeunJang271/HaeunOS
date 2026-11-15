# Physical memory and virtual board

This document specifies the intended initial board. The VM implements flat RAM, Boot ROM write-protect, UART, block, timer, IRQC, framebuffer control, keyboard, mouse, and optional MMU translation ([mmu-v1.md](mmu-v1.md)). The authoritative address table is [architecture/memory-map.md](architecture/memory-map.md). Addresses are guest physical unless a guest enables paging.

## Physical map (256 MiB RAM profile)

| Range | Region | v1 contract |
|---|---|---|
| `0x00000000–0x0FFFFFFF` | 256 MiB RAM | byte-addressed; Boot ROM code window write-protected after HFW load |
| `0x10000000–0x10000FFF` | interrupt controller | MMIO, 4 KiB aperture |
| `0x10001000–0x10001FFF` | timer | MMIO, 4 KiB aperture |
| `0x10002000–0x10002FFF` | UART | MMIO, 4 KiB aperture |
| `0x10003000–0x10003FFF` | block device | MMIO, 4 KiB aperture |
| `0x10004000–0x10004FFF` | framebuffer control | MMIO, 4 KiB aperture |
| `0x10005000–0x10005FFF` | keyboard | MMIO, 4 KiB aperture |
| `0x10006000–0x10006FFF` | mouse | MMIO, 4 KiB aperture |
| all remaining addresses | unmapped | bus fault |

The address map is chosen outside RAM, so board physical capacity does not consume the 32-bit address space. Firmware occupies RAM `0x00000000–0x0000FFFF`; kernel load address is `0x00100000`; initial stack reservation is `0x0FF00000–0x0FFFFFFF`, with initial stack pointer `0x10000000`. The VM CLI initializes `r13` to the exclusive stack top and `r14` to the inclusive lower bound. The C subset checks proposed frame and outgoing-argument allocations against `r14` and stops through ECALL on overflow. This is compiler-generated ABI protection: arbitrary guest stores and hand-written assembly are not contained by a VM memory-region checker. The prototype linker keeps image segments below the stack reservation.

## Virtual map target

Virtual address space is 4 GiB, 4 KiB pages. Software convention: `0x00000000–0xBFFFFFFF` user region, `0xC0000000–0xFFFFFFFF` supervisor region. User access requires the PTE `U` bit. Page table format, `ptbr`, and page-fault cause `5` are specified in [mmu-v1.md](mmu-v1.md). With paging disabled (reset), `VA=PA`. After paging is enabled, RAM and MMIO are reached only through translations (or physical DMA programming).

## MMIO register contract

MMIO accesses are 32-bit aligned little-endian words. Reserved registers read zero and ignore writes; reserved bits written as one are ignored. Accesses of wrong width/alignment fault. Reset clears device state. Device offsets are byte offsets in each aperture. The VM routes guest LW/SW accesses to RAM or a mapped device; an unimplemented device aperture or address outside RAM and implemented apertures stops with the VM memory-fault result.

### Interrupt controller (`0x10000000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | PENDING (RO) | bit 0 timer, bit 1 UART RX, bit 2 block completion; latched pending bits |
| 0x04 | ENABLE (RW) | per-source mask bits 2:0; reset 0; other bits ignored |
| 0x08 | CLAIM (RO) | lowest-numbered source that is both pending and enabled, or `0xFFFFFFFF` if none |
| 0x0C | COMPLETE (WO) | writing source ID `0..2` clears that edge-pending latch; other values ignored |

Pending bits are edge latches:

- Timer: set when `COUNT >= COMPARE` while CONTROL bit 0 is set; hardware then clears CONTROL bit 0.
- UART RX: set when RX interrupt enable is set and the RX FIFO is nonempty (on feed, on enabling CONTROL, or when COMPLETE left data still available only after a new enable/feed edge).
- Block: set when a COMMAND completes (STATUS becomes done or error) while IRQ_ENABLE bit 0 is set.

CPU interrupt delivery still requires ISA v4 `status.IE` sampling; MMIO alone does not vector.

### Timer (`0x10001000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | COUNT_LO (RO) | low 32 bits of monotonic virtual tick counter |
| 0x04 | COUNT_HI (RO) | high 32 bits |
| 0x08 | COMPARE_LO (RW) | low compare bits |
| 0x0C | COMPARE_HI (RW) | high compare bits |
| 0x10 | CONTROL (RW) | bit 0 enables one-shot interrupt; writing 0 disables; reset 0 |

Virtual time advances deterministically by one tick per retired guest instruction. When CONTROL bit 0 is set and `COUNT >= COMPARE`, IRQC source 0 is latched pending and CONTROL bit 0 clears.

### UART (`0x10002000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | DATA (RW) | write low byte appends output; read removes next input byte, or returns bit 31 set when empty |
| 0x04 | STATUS (RO) | bit 0 input available, bit 1 output sink ready (always 1); other bits zero |
| 0x08 | CONTROL (RW) | bit 0 RX interrupt enable; reset 0 |

The VM implements all three UART registers. RX is a 4096-byte FIFO supplied through the host API; a feed request that exceeds available capacity is rejected atomically. DATA reads pop one byte or return bit 31 set when empty. DATA writes send the low byte to the configured host sink; without a sink the byte is discarded. STATUS bit 1 remains set because TX is always ready. CONTROL bit 0 enables the UART RX pending latch described above; CPU delivery still requires ISA v4. Reserved aligned word offsets in the UART aperture read zero and ignore writes.

### Block device (`0x10003000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | STATUS (RO) | 0 idle, 1 busy, 2 done, 3 error |
| 0x04 | COMMAND (WO) | 1 read block, 2 write block; other values set error |
| 0x08 | LBA (RW) | 32-bit logical block address |
| 0x0C | DMA_ADDR (RW) | guest physical destination/source, 512-byte aligned |
| 0x10 | IRQ_ENABLE (RW) | bit 0 completion interrupt |

Block size is 512 bytes. Commands validate the complete DMA range against RAM and fail atomically on invalid range (STATUS=3, no partial copy). The virtual disk is a host-supplied raw byte array whose length must be a multiple of 512. Completion is synchronous in v1 (STATUS goes idle→busy→done/error in one COMMAND write). IRQ_ENABLE latches IRQC source 2 on completion; CPU delivery requires ISA v4.

## Reset and interrupt boundary

Reset PC is `0x00000000` in firmware. Global CPU interrupt enable is clear until ISA v4 `status.IE` is set. Board IRQC/timer MMIO and between-instruction interrupt sampling/trap vectoring follow [isa-v4.md](isa-v4.md).
