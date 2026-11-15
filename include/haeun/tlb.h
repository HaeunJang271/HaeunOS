#ifndef HAEUN_TLB_H
#define HAEUN_TLB_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Invalidates every TLB entry. Safe on a NULL vm (no-op).
 */
void haeun_tlb_flush(haeun_vm *vm);

/**
 * Looks up vpn in the TLB. On hit stores the cached leaf PTE and returns true.
 */
bool haeun_tlb_lookup(const haeun_vm *vm, uint32_t vpn, uint32_t *pte);

/**
 * Inserts or replaces a leaf translation for vpn using round-robin replacement.
 */
void haeun_tlb_insert(haeun_vm *vm, uint32_t vpn, uint32_t pte);

#endif
