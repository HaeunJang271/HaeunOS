# HAEUN TLB v1

Status: **specified, implemented, and tested**. Depends on [mmu-v1.md](mmu-v1.md).

## Goal

Cache leaf translations so repeated accesses to the same VPN avoid a full two-level walk, while keeping permission checks identical to a walk.

## Organization

- Fully associative, **16** entries.
- Each valid entry stores `VPN` (`VA[31:12]`) and the leaf PTE word (flags + PPN).
- Replacement is round-robin.
- Bare mode (`ptbr.MODE=0`) does not use the TLB.

## Lookup

On a translated access with paging enabled:

1. Search for a valid entry with matching VPN.
2. On hit, apply the same leaf permission checks as a walk (`R`/`W`/`X`, user/`U`). Success yields `PA = (PPN<<12) | VA[11:0]`.
3. On miss, walk the page tables; on a successful leaf, insert (or replace) a TLB entry, then return.

Failed walks and permission failures do not insert entries.

## Invalidation

Any successful write to CSR `ptbr` (including rewriting the same value) flushes all TLB entries. Supervisor `SFENCE` (`op=0x12`, ISA v4) also flushes the entire TLB without changing `ptbr`. After editing PTEs in memory, software must either rewrite `ptbr` or execute `SFENCE` before relying on the new mapping.

`haeun_vm_init*` clears the TLB with the rest of VM state.

## Incremental delivery

1. Spec + 16-entry TLB + `ptbr` flush (done).
2. Supervisor `SFENCE` full-flush opcode (done).
3. Optional later: ASID or selective invalidate.
