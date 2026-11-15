#include "haeun/instruction_executor.h"
#include "haeun/board_mmio.h"
#include "haeun/cpu_state.h"
#include "haeun/exception_handler.h"
#include "haeun/memory_interface.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

static bool signed_less(uint32_t a, uint32_t b) {
    bool sign_a = (a & UINT32_C(0x80000000)) != 0u;
    bool sign_b = (b & UINT32_C(0x80000000)) != 0u;
    return sign_a != sign_b ? sign_a : a < b;
}

static void retire(haeun_vm *vm, uint32_t next_pc) {
    haeun_rf_clear_r0(vm);
    haeun_cpu_set_pc(vm, next_pc);
    haeun_board_on_retire(vm);
    (void)haeun_trap_try_interrupt(vm);
}

/**
 * Applies one decoded instruction to CPU and memory state.
 */
haeun_vm_result haeun_execute_instruction(haeun_vm *vm,
                                          const haeun_decoded_insn *insn) {
    uint32_t pc, next, address, trap_cause = 0u, trap_tval = 0u, value;
    uint8_t op, rd, ra, rb;
    int32_t imm;
    if (vm == NULL || insn == NULL) return HAEUN_VM_BAD_ARGUMENT;
    pc = haeun_cpu_pc(vm);
    next = pc + 4u;
    op = insn->op;
    rd = insn->rd;
    ra = insn->ra;
    rb = insn->rb;
    imm = insn->imm;
    switch (op) {
    case HAEUN_OP_NOP:
        break;
    case HAEUN_OP_HALT:
        haeun_cpu_set_halted(vm, true);
        haeun_rf_clear_r0(vm);
        haeun_cpu_set_pc(vm, next);
        haeun_board_on_retire(vm);
        return HAEUN_VM_HALTED;
    case HAEUN_OP_ADD:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) + haeun_rf_read(vm, rb));
        break;
    case HAEUN_OP_SUB:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) - haeun_rf_read(vm, rb));
        break;
    case HAEUN_OP_AND:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) & haeun_rf_read(vm, rb));
        break;
    case HAEUN_OP_OR:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) | haeun_rf_read(vm, rb));
        break;
    case HAEUN_OP_XOR:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) ^ haeun_rf_read(vm, rb));
        break;
    case HAEUN_OP_ADDI:
        haeun_rf_write(vm, rd, haeun_rf_read(vm, ra) + (uint32_t)imm);
        break;
    case HAEUN_OP_LW:
        address = haeun_rf_read(vm, ra) + (uint32_t)imm;
        if (!haeun_mem_load_u32(vm, address, &value, &trap_cause, &trap_tval))
            return haeun_exception_raise(
                vm,
                trap_cause == 2u ? HAEUN_VM_ALIGNMENT_FAULT
                                 : HAEUN_VM_MEMORY_FAULT,
                trap_cause, trap_tval);
        haeun_rf_write(vm, rd, value);
        break;
    case HAEUN_OP_SW:
        address = haeun_rf_read(vm, ra) + (uint32_t)imm;
        if (!haeun_mem_store_u32(vm, address, haeun_rf_read(vm, rd), &trap_cause,
                                 &trap_tval))
            return haeun_exception_raise(
                vm,
                trap_cause == 2u ? HAEUN_VM_ALIGNMENT_FAULT
                                 : HAEUN_VM_MEMORY_FAULT,
                trap_cause, trap_tval);
        break;
    case HAEUN_OP_BEQ:
        if (haeun_rf_read(vm, rd) == haeun_rf_read(vm, ra))
            next = pc + 4u + (uint32_t)(imm * 4);
        break;
    case HAEUN_OP_BNE:
        if (haeun_rf_read(vm, rd) != haeun_rf_read(vm, ra))
            next = pc + 4u + (uint32_t)(imm * 4);
        break;
    case HAEUN_OP_JAL:
        haeun_rf_write(vm, rd, pc + 4u);
        next = pc + 4u + (uint32_t)(imm * 4);
        break;
    case HAEUN_OP_JALR:
        address = haeun_rf_read(vm, ra) + (uint32_t)imm;
        haeun_rf_write(vm, rd, pc + 4u);
        next = address;
        break;
    case HAEUN_OP_CMP:
        switch (insn->cmp_cond) {
        case 0u:
            value = (haeun_rf_read(vm, ra) == haeun_rf_read(vm, rb)) ? 1u : 0u;
            break;
        case 1u:
            value = (haeun_rf_read(vm, ra) != haeun_rf_read(vm, rb)) ? 1u : 0u;
            break;
        case 2u:
            value = signed_less(haeun_rf_read(vm, ra), haeun_rf_read(vm, rb))
                        ? 1u
                        : 0u;
            break;
        case 3u:
            value = (!signed_less(haeun_rf_read(vm, rb), haeun_rf_read(vm, ra)))
                        ? 1u
                        : 0u;
            break;
        case 4u:
            value = signed_less(haeun_rf_read(vm, rb), haeun_rf_read(vm, ra))
                        ? 1u
                        : 0u;
            break;
        case 5u:
            value = (!signed_less(haeun_rf_read(vm, ra), haeun_rf_read(vm, rb)))
                        ? 1u
                        : 0u;
            break;
        default:
            return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION, 0u,
                                         0u);
        }
        haeun_rf_write(vm, rd, value);
        break;
    case HAEUN_OP_ECALL:
        /* Security M3: gate user syscalls; poison r1 on deny, still trap. */
        (void)haeun_sec_ecall_before_trap(vm);
        /* Process-HFS: optional host I/O assist completes OPEN path, etc. */
        if (vm->ecall_io != NULL &&
            vm->ecall_io(vm, vm->ecall_io_context)) {
            retire(vm, next);
            return HAEUN_VM_OK;
        }
        return haeun_exception_raise(vm, HAEUN_VM_TRAP, 3u, 0u);
    case HAEUN_OP_CSR:
        if (!haeun_trap_csr(vm, rd, ra, (uint32_t)(insn->raw & 0x3ffffu),
                            &trap_cause, &trap_tval))
            return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION,
                                         trap_cause, trap_tval);
        break;
    case HAEUN_OP_ERET:
        if (!haeun_trap_eret(vm, &trap_cause, &trap_tval))
            return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION,
                                         trap_cause, trap_tval);
        haeun_rf_clear_r0(vm);
        haeun_board_on_retire(vm);
        (void)haeun_trap_try_interrupt(vm);
        return HAEUN_VM_OK;
    case HAEUN_OP_SFENCE:
        if (!haeun_trap_sfence(vm, &trap_cause, &trap_tval))
            return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION,
                                         trap_cause, trap_tval);
        break;
    default:
        return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION, 0u, 0u);
    }
    retire(vm, next);
    return HAEUN_VM_OK;
}
