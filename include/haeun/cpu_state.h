#ifndef HAEUN_CPU_STATE_H
#define HAEUN_CPU_STATE_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Resets CPU architectural fields inside vm (not board device FIFOs).
 * Caller must already have validated ram/entry/isa.
 */
void haeun_cpu_reset(haeun_vm *vm, uint32_t entry_pc, uint32_t isa_version);

/**
 * Returns true when the CPU has halted.
 */
bool haeun_cpu_halted(const haeun_vm *vm);

/**
 * Marks the CPU halted.
 */
void haeun_cpu_set_halted(haeun_vm *vm, bool halted);

/**
 * Returns the current program counter.
 */
uint32_t haeun_cpu_pc(const haeun_vm *vm);

/**
 * Sets the program counter.
 */
void haeun_cpu_set_pc(haeun_vm *vm, uint32_t pc);

#endif
