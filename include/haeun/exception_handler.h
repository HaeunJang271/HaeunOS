#ifndef HAEUN_EXCEPTION_HANDLER_H
#define HAEUN_EXCEPTION_HANDLER_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Raises a synchronous fault. Under ISA v4 enters the guest trap path and
 * returns HAEUN_VM_OK; earlier ISAs stop with host_result.
 */
haeun_vm_result haeun_exception_raise(haeun_vm *vm, haeun_vm_result host_result,
                                      uint32_t cause, uint32_t tval);

/** Enters the ISA v4 trap path and jumps to trapvec. */
void haeun_trap_enter(haeun_vm *vm, uint32_t epc, uint32_t cause, uint32_t tval);

/**
 * Nonzero when cause is a fatal user synchronous fault (0/1/2/4/5),
 * excluding ECALL (3) and timer/IRQ (>=16).
 */
int haeun_trap_cause_is_user_fatal(uint32_t cause);

/** Delivers a pending enabled IRQC interrupt when status.IE is set. */
bool haeun_trap_try_interrupt(haeun_vm *vm);

/**
 * Executes a CSR instruction. Returns false when the instruction must
 * take a synchronous fault (*cause and *tval filled).
 */
bool haeun_trap_csr(haeun_vm *vm, uint8_t rd, uint8_t ra, uint32_t imm18,
                    uint32_t *cause, uint32_t *tval);

/**
 * Executes ERET. Returns false when the instruction must take a
 * synchronous fault (*cause filled, *tval zero).
 */
bool haeun_trap_eret(haeun_vm *vm, uint32_t *cause, uint32_t *tval);

/**
 * Executes SFENCE (full TLB flush). Returns false on privilege fault.
 */
bool haeun_trap_sfence(haeun_vm *vm, uint32_t *cause, uint32_t *tval);

#endif
