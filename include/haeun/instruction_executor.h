#ifndef HAEUN_INSTRUCTION_EXECUTOR_H
#define HAEUN_INSTRUCTION_EXECUTOR_H

#include "haeun/instruction_decoder.h"
#include "haeun/vm.h"

/**
 * Executes one already-decoded instruction. Updates PC / halt / traps as
 * defined by the ISA. On success-with-retire returns HAEUN_VM_OK or HALTED.
 */
haeun_vm_result haeun_execute_instruction(haeun_vm *vm,
                                          const haeun_decoded_insn *insn);

#endif
