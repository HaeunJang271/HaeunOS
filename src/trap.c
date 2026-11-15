#include "haeun/exception_handler.h"
#include "haeun/board_mmio.h"
#include "haeun/tlb.h"

#include <stdio.h>

/**
 * Fatal user synchronous causes: illegal, memory, align, privilege, page fault.
 */
int haeun_trap_cause_is_user_fatal(uint32_t cause) {
    return cause == 0u || cause == 1u || cause == 2u || cause == 4u ||
           cause == 5u;
}

enum {
    CSR_STATUS = 0u,
    CSR_TRAPVEC = 1u,
    CSR_EPC = 2u,
    CSR_CAUSE = 3u,
    CSR_TVAL = 4u,
    CSR_SCRATCH = 5u,
    CSR_PTBR = 6u,
    PTBR_RES_MASK = 0xffeu,
    STATUS_IE = 1u,
    STATUS_PIE = 2u,
    STATUS_PP_SHIFT = 2u
};

static uint32_t status_masked(uint32_t value) {
    return (value & (STATUS_IE | STATUS_PIE)) |
           (((value >> STATUS_PP_SHIFT) & 1u) << STATUS_PP_SHIFT);
}

static uint32_t csr_read(const haeun_vm *vm, uint32_t csr) {
    switch (csr) {
    case CSR_STATUS: return vm->csr_status;
    case CSR_TRAPVEC: return vm->csr_trapvec;
    case CSR_EPC: return vm->csr_epc;
    case CSR_CAUSE: return vm->csr_cause;
    case CSR_TVAL: return vm->csr_tval;
    case CSR_SCRATCH: return vm->csr_scratch;
    case CSR_PTBR: return vm->csr_ptbr;
    default: return 0u;
    }
}

static bool csr_write(haeun_vm *vm, uint32_t csr, uint32_t value) {
    switch (csr) {
    case CSR_STATUS:
        vm->csr_status = status_masked(value);
        return true;
    case CSR_TRAPVEC:
        if ((value & 3u) != 0u) return false;
        vm->csr_trapvec = value;
        return true;
    case CSR_EPC: vm->csr_epc = value; return true;
    case CSR_CAUSE: vm->csr_cause = value; return true;
    case CSR_TVAL: vm->csr_tval = value; return true;
    case CSR_SCRATCH: vm->csr_scratch = value; return true;
    case CSR_PTBR:
        if ((value & PTBR_RES_MASK) != 0u) return false;
        vm->csr_ptbr = value;
        haeun_tlb_flush(vm);
        return true;
    default: return false;
    }
}

static bool known_csr(uint32_t csr) {
    return csr <= CSR_PTBR;
}

void haeun_trap_enter(haeun_vm *vm, uint32_t epc, uint32_t cause, uint32_t tval) {
    uint32_t status = vm->csr_status;
    uint32_t prev_priv = vm->privilege;
    vm->csr_epc = epc;
    vm->csr_cause = cause;
    vm->csr_tval = tval;
    status = (status & ~STATUS_PIE) |
             (((status & STATUS_IE) != 0u) ? STATUS_PIE : 0u);
    status &= ~STATUS_IE;
    status &= ~(1u << STATUS_PP_SHIFT);
    status |= (prev_priv & 1u) << STATUS_PP_SHIFT;
    vm->csr_status = status;
    vm->privilege = HAEUN_PRIV_SUPERVISOR;
    vm->pc = vm->csr_trapvec;
    vm->cause = cause;
    vm->fault_pc = epc;
    if (prev_priv == HAEUN_PRIV_USER &&
        haeun_trap_cause_is_user_fatal(cause) && vm->fault_io != NULL)
        vm->fault_io(vm, vm->fault_io_context, cause, tval, epc);
    else if (prev_priv == HAEUN_PRIV_SUPERVISOR &&
             haeun_trap_cause_is_user_fatal(cause)) {
        (void)fprintf(stderr,
                      "[haeun-fault] KERNEL cause=%u epc=0x%08x tval=0x%08x\n",
                      cause, epc, tval);
    }
}

bool haeun_trap_try_interrupt(haeun_vm *vm) {
    uint32_t id;
    if (vm == NULL || vm->isa_version < HAEUN_ISA_VERSION_4) return false;
    if ((vm->csr_status & STATUS_IE) == 0u) return false;
    id = haeun_board_irq_claim(vm);
    if (id == UINT32_C(0xffffffff)) return false;
    haeun_trap_enter(vm, vm->pc, 16u + id, 0u);
    return true;
}

bool haeun_trap_csr(haeun_vm *vm, uint8_t rd, uint8_t ra, uint32_t imm18,
                    uint32_t *cause, uint32_t *tval) {
    uint32_t csr = imm18 & 0xfffu;
    uint32_t funct = (imm18 >> 12) & 3u;
    uint32_t old;
    *tval = 0u;
    if (vm->privilege != HAEUN_PRIV_SUPERVISOR) {
        *cause = 4u;
        return false;
    }
    if (funct == 0u || (imm18 & 0x3c000u) != 0u || !known_csr(csr)) {
        *cause = 0u;
        return false;
    }
    old = csr_read(vm, csr);
    if (rd != 0u) vm->r[rd] = old;
    if (funct == 1u) {
        if (!csr_write(vm, csr, vm->r[ra])) {
            *cause = 0u;
            return false;
        }
    } else if (ra != 0u) {
        uint32_t next = (funct == 2u) ? (old | vm->r[ra]) : (old & ~vm->r[ra]);
        if (!csr_write(vm, csr, next)) {
            *cause = 0u;
            return false;
        }
    }
    return true;
}

bool haeun_trap_eret(haeun_vm *vm, uint32_t *cause, uint32_t *tval) {
    uint32_t status;
    *tval = 0u;
    if (vm->privilege != HAEUN_PRIV_SUPERVISOR) {
        *cause = 4u;
        return false;
    }
    status = vm->csr_status;
    vm->pc = vm->csr_epc;
    vm->privilege = (status >> STATUS_PP_SHIFT) & 1u;
    if ((status & STATUS_PIE) != 0u) status |= STATUS_IE;
    else status &= ~STATUS_IE;
    status &= ~(STATUS_PIE | (1u << STATUS_PP_SHIFT));
    vm->csr_status = status;
    return true;
}

/**
 * Flushes the TLB when executed in supervisor mode.
 */
bool haeun_trap_sfence(haeun_vm *vm, uint32_t *cause, uint32_t *tval) {
    *tval = 0u;
    if (vm->privilege != HAEUN_PRIV_SUPERVISOR) {
        *cause = 4u;
        return false;
    }
    haeun_tlb_flush(vm);
    return true;
}

/**
 * Raises a synchronous exception or host stop, depending on ISA version.
 */
haeun_vm_result haeun_exception_raise(haeun_vm *vm, haeun_vm_result host_result,
                                      uint32_t cause, uint32_t tval) {
    if (vm == NULL) return HAEUN_VM_BAD_ARGUMENT;
    if (vm->isa_version >= HAEUN_ISA_VERSION_4) {
        haeun_trap_enter(vm, vm->pc, cause, tval);
        return HAEUN_VM_OK;
    }
    vm->fault_pc = vm->pc;
    vm->cause = cause;
    return host_result;
}
