#ifndef HAEUN_ISA_OPCODES_H
#define HAEUN_ISA_OPCODES_H

/* Generated from tools/isa/haeun_isa.json — do not edit by hand. */
/* Run: python tools/gen_isa_opcodes.py */

enum {
    HAEUN_OP_NOP = 0,
    HAEUN_OP_HALT = 1,
    HAEUN_OP_ADD = 2,
    HAEUN_OP_SUB = 3,
    HAEUN_OP_AND = 4,
    HAEUN_OP_OR = 5,
    HAEUN_OP_XOR = 6,
    HAEUN_OP_ADDI = 7,
    HAEUN_OP_LW = 8,
    HAEUN_OP_SW = 9,
    HAEUN_OP_BEQ = 10,
    HAEUN_OP_BNE = 11,
    HAEUN_OP_JAL = 12,
    HAEUN_OP_ECALL = 13,
    HAEUN_OP_JALR = 14,
    HAEUN_OP_CMP = 15,
    HAEUN_OP_CSR = 16,
    HAEUN_OP_ERET = 17,
    HAEUN_OP_SFENCE = 18,
};

#endif
