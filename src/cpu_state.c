#include "haeun/cpu_state.h"

/**
 * Initializes CPU fields to reset defaults for the given ISA version.
 */
void haeun_cpu_reset(haeun_vm *vm, uint32_t entry_pc, uint32_t isa_version) {
    if (vm == NULL) return;
    vm->pc = entry_pc;
    vm->isa_version = isa_version;
    vm->privilege = HAEUN_PRIV_SUPERVISOR;
    vm->cause = 0u;
    vm->fault_pc = 0u;
    vm->halted = false;
    vm->csr_status = 0u;
    vm->csr_trapvec = 0u;
    vm->csr_epc = 0u;
    vm->csr_cause = 0u;
    vm->csr_tval = 0u;
    vm->csr_scratch = 0u;
    vm->csr_ptbr = 0u;
}

/**
 * Reports whether the CPU is in the halted state.
 */
bool haeun_cpu_halted(const haeun_vm *vm) {
    return vm != NULL && vm->halted;
}

/**
 * Sets or clears the halted flag.
 */
void haeun_cpu_set_halted(haeun_vm *vm, bool halted) {
    if (vm == NULL) return;
    vm->halted = halted;
}

/**
 * Returns the architectural program counter.
 */
uint32_t haeun_cpu_pc(const haeun_vm *vm) {
    return vm == NULL ? 0u : vm->pc;
}

/**
 * Updates the architectural program counter.
 */
void haeun_cpu_set_pc(haeun_vm *vm, uint32_t pc) {
    if (vm == NULL) return;
    vm->pc = pc;
}
