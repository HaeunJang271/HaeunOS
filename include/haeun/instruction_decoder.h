#ifndef HAEUN_INSTRUCTION_DECODER_H
#define HAEUN_INSTRUCTION_DECODER_H

#include "haeun/isa_opcodes.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t raw;
    uint8_t op;
    uint8_t rd;
    uint8_t ra;
    uint8_t rb;
    uint8_t cmp_cond;
    int32_t imm;
} haeun_decoded_insn;

/**
 * Decodes insn for isa_version. On success fills *out and returns true.
 * On illegal encoding sets *cause to 0 and returns false.
 */
bool haeun_decode_instruction(uint32_t insn, uint32_t isa_version,
                              haeun_decoded_insn *out, uint32_t *cause);

#endif
