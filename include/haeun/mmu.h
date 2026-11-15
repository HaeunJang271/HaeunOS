#ifndef HAEUN_MMU_H
#define HAEUN_MMU_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

#define HAEUN_MMU_ACCESS_R 1u
#define HAEUN_MMU_ACCESS_W 2u
#define HAEUN_MMU_ACCESS_X 4u
#define HAEUN_CAUSE_PAGE_FAULT 5u

/**
 * Translates a guest virtual address for the given access kind.
 * On success stores the physical address in *pa and returns true.
 * On failure stores the architectural cause in *cause and returns false.
 */
bool haeun_mmu_translate(haeun_vm *vm, uint32_t va, uint32_t access,
                         uint32_t *pa, uint32_t *cause);

#endif
