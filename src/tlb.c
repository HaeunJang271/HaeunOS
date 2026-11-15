#include "haeun/tlb.h"

/**
 * Clears all TLB valid bits and resets the round-robin cursor.
 */
void haeun_tlb_flush(haeun_vm *vm) {
    uint32_t i;
    if (vm == NULL) return;
    for (i = 0u; i < HAEUN_TLB_ENTRIES; ++i) {
        vm->tlb_valid[i] = false;
        vm->tlb_vpn[i] = 0u;
        vm->tlb_pte[i] = 0u;
    }
    vm->tlb_next = 0u;
}

/**
 * Returns a cached leaf PTE for vpn when a valid matching entry exists.
 */
bool haeun_tlb_lookup(const haeun_vm *vm, uint32_t vpn, uint32_t *pte) {
    uint32_t i;
    if (vm == NULL || pte == NULL) return false;
    for (i = 0u; i < HAEUN_TLB_ENTRIES; ++i) {
        if (vm->tlb_valid[i] && vm->tlb_vpn[i] == vpn) {
            *pte = vm->tlb_pte[i];
            return true;
        }
    }
    return false;
}

/**
 * Stores vpn/pte in the next round-robin slot.
 */
void haeun_tlb_insert(haeun_vm *vm, uint32_t vpn, uint32_t pte) {
    uint32_t slot;
    if (vm == NULL) return;
    slot = vm->tlb_next % HAEUN_TLB_ENTRIES;
    vm->tlb_vpn[slot] = vpn;
    vm->tlb_pte[slot] = pte;
    vm->tlb_valid[slot] = true;
    vm->tlb_next = (slot + 1u) % HAEUN_TLB_ENTRIES;
}
