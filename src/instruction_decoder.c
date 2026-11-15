#include "haeun/instruction_decoder.h"
#include "haeun/vm.h"

static int32_t sign_extend18(uint32_t value) {
    uint32_t sign = UINT32_C(1) << 17;
    uint32_t mask = (UINT32_C(1) << 18) - 1u;
    value &= mask;
    if ((value & sign) == 0u) return (int32_t)value;
    return -(int32_t)(((~value) & mask) + 1u);
}

/**
 * Validates encoding and extracts fields for the selected ISA version.
 */
bool haeun_decode_instruction(uint32_t insn, uint32_t isa_version,
                              haeun_decoded_insn *out, uint32_t *cause) {
    uint8_t op, rd, ra, rb, cond;
    if (out == NULL || cause == NULL) {
        if (cause != NULL) *cause = 0u;
        return false;
    }
    op = (uint8_t)(insn >> 26);
    rd = (uint8_t)((insn >> 22) & 15u);
    ra = (uint8_t)((insn >> 18) & 15u);
    rb = (uint8_t)((insn >> 14) & 15u);
    cond = (uint8_t)((insn >> 11) & 7u);
    *cause = 0u;
    if (((op == HAEUN_OP_NOP || op == HAEUN_OP_HALT || op == HAEUN_OP_ECALL ||
          op == HAEUN_OP_ERET || op == HAEUN_OP_SFENCE) &&
         (insn & UINT32_C(0x03ffffff)) != 0u) ||
        ((op >= HAEUN_OP_ADD && op <= HAEUN_OP_XOR) &&
         (insn & UINT32_C(0x00003fff)) != 0u) ||
        (op == HAEUN_OP_JAL && (insn & UINT32_C(0x003c0000)) != 0u)) {
        return false;
    }
    if (op == HAEUN_OP_JALR && isa_version < HAEUN_ISA_VERSION_2) return false;
    if (op == HAEUN_OP_CMP &&
        (isa_version < HAEUN_ISA_VERSION_3 ||
         (insn & UINT32_C(0x000007ff)) != 0u || cond > 5u)) {
        return false;
    }
    if ((op == HAEUN_OP_CSR || op == HAEUN_OP_ERET || op == HAEUN_OP_SFENCE) &&
        isa_version < HAEUN_ISA_VERSION_4) {
        return false;
    }
    if (op > HAEUN_OP_SFENCE) return false;
    out->raw = insn;
    out->op = op;
    out->rd = rd;
    out->ra = ra;
    out->rb = rb;
    out->cmp_cond = cond;
    out->imm = sign_extend18(insn & UINT32_C(0x3ffff));
    return true;
}
