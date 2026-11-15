# HAEUN MMU and page tables v1

Status: **specified; bare/Sv32-style walk and TLB v1 implemented and tested** under ISA v4. Depends on [platform.md](platform.md), [isa-v4.md](isa-v4.md), and [tlb-v1.md](tlb-v1.md).

## Goal

Translate guest virtual addresses to physical addresses when paging is enabled, enforce per-page R/W/X and user permissions, and deliver page faults through the ISA v4 trap path.

## Address spaces

- Virtual and physical addresses are 32-bit.
- Page size is 4 KiB. Page offset is `VA[11:0]`.
- Two-level page tables (Sv32-style indices): L1 index `VA[31:22]`, L0 index `VA[21:12]`.
- Software map: user `0x00000000–0xBFFFFFFF`, supervisor `0xC0000000–0xFFFFFFFF` (convention; the MMU does not hard-code these ranges).

## CSR `ptbr` (`0x006`)

Supervisor-only, ISA v4+.

| Bits | Field | Meaning |
|---:|---|---|
| 0 | MODE | `0` bare (`VA=PA`); `1` page walk enabled |
| 11:1 | reserved | must be written zero; nonzero write is illegal and leaves `ptbr` unchanged |
| 31:12 | PPN | physical page number of the root (L1) table |

Reset value is `0` (bare). Instruction fetch and `LW`/`SW` use translation only when `MODE=1`. Page-table walks always read PTE words with **physical** addresses into guest RAM. MMIO and block DMA remain physical after translation of the programmed addresses; devices are not remapped by the MMIO decoder.

## PTE format (32-bit, little-endian)

| Bits | Name | Meaning |
|---:|---|---|
| 0 | V | valid |
| 1 | R | readable |
| 2 | W | writable |
| 3 | X | executable |
| 4 | U | user-accessible |
| 11:5 | reserved | must be zero on valid PTEs; nonzero → page fault |
| 31:12 | PPN | next-level table (non-leaf) or leaf physical page |

A PTE with `V=1` and `R=W=X=0` is a non-leaf pointer to the next table page (`PA = PPN<<12`). A leaf must set at least one of `R`, `W`, `X`. `W=1` with `R=0` is invalid (page fault). Megapages (leaf at L1) are not supported in v1: an L1 leaf causes a page fault.

## Permission checks

Access kinds: read (`LW`), write (`SW`), execute (instruction fetch).

- Missing `V`, bad reserved bits, invalid leaf/non-leaf shape, or missing permission bit → page fault (cause `5`), `tval=VA`.
- User mode requires `U=1` on the leaf; otherwise page fault.
- Supervisor may access pages with `U=0` or `U=1` (no separate SUM bit in v1).

## Fault delivery

Cause `5` is a page fault. Under ISA v4 it enters the guest trap path with `tval` equal to the faulting virtual address. Bare-mode out-of-range/unmapped physical accesses keep cause `1` (memory fault). Alignment faults remain cause `2` and are checked on the virtual address for `LW`/`SW` before translation.

## TLB

A 16-entry fully associative TLB caches successful leaf translations. Permission checks on a hit match a walk. Any write to `ptbr`, or a supervisor `SFENCE`, flushes the TLB; after PTE edits, software must rewrite `ptbr` or execute `SFENCE` (see [tlb-v1.md](tlb-v1.md)).

## Incremental delivery

1. Ratify this document; implement `ptbr`, two-level walk, fetch/`LW`/`SW` translation, and page-fault traps.
2. Host page-map helpers (`haeun_pt_*`) to build identity/user leaves (done; see [kernel-v1.md](kernel-v1.md)).
3. TLB v1 with `ptbr` flush (done; see [tlb-v1.md](tlb-v1.md)).
