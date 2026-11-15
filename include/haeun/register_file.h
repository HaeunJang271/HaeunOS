#ifndef HAEUN_REGISTER_FILE_H
#define HAEUN_REGISTER_FILE_H

#include "haeun/vm.h"

#include <stdint.h>

/**
 * Reads GPR idx (0..15). Out-of-range idx returns 0.
 */
uint32_t haeun_rf_read(const haeun_vm *vm, unsigned idx);

/**
 * Writes GPR idx. Ignores writes to r0 and out-of-range indices.
 */
void haeun_rf_write(haeun_vm *vm, unsigned idx, uint32_t value);

/**
 * Forces r0 to zero after a retired instruction.
 */
void haeun_rf_clear_r0(haeun_vm *vm);

#endif
