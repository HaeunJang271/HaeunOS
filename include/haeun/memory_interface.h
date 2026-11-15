#ifndef HAEUN_MEMORY_INTERFACE_H
#define HAEUN_MEMORY_INTERFACE_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Fetches one instruction word at virtual address va.
 * On fault returns false and fills *cause / *tval.
 */
bool haeun_mem_fetch(haeun_vm *vm, uint32_t va, uint32_t *word,
                     uint32_t *cause, uint32_t *tval);

/**
 * Loads a 32-bit word from virtual address va (must be aligned).
 */
bool haeun_mem_load_u32(haeun_vm *vm, uint32_t va, uint32_t *value,
                        uint32_t *cause, uint32_t *tval);

/**
 * Stores a 32-bit word to virtual address va (must be aligned).
 */
bool haeun_mem_store_u32(haeun_vm *vm, uint32_t va, uint32_t value,
                         uint32_t *cause, uint32_t *tval);

#endif
