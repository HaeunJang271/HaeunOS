#include "haeun/register_file.h"

/**
 * Reads a general-purpose register with bounds checking.
 */
uint32_t haeun_rf_read(const haeun_vm *vm, unsigned idx) {
    if (vm == NULL || idx >= HAEUN_REGISTER_COUNT) return 0u;
    return vm->r[idx];
}

/**
 * Writes a general-purpose register; r0 remains zero.
 */
void haeun_rf_write(haeun_vm *vm, unsigned idx, uint32_t value) {
    if (vm == NULL || idx == 0u || idx >= HAEUN_REGISTER_COUNT) return;
    vm->r[idx] = value;
}

/**
 * Clears r0 after instruction retirement.
 */
void haeun_rf_clear_r0(haeun_vm *vm) {
    if (vm == NULL) return;
    vm->r[0] = 0u;
}
