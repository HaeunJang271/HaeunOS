#ifndef HAEUN_INSTRUCTION_TRACE_H
#define HAEUN_INSTRUCTION_TRACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HAEUN_TRACE_CAPACITY 32u

typedef struct {
    uint32_t pc;
    uint32_t insn;
    uint32_t next_pc;
    uint8_t op;
} haeun_trace_entry;

struct haeun_instruction_trace {
    haeun_trace_entry entries[HAEUN_TRACE_CAPACITY];
    uint32_t head;
    uint32_t count;
};

typedef struct haeun_instruction_trace haeun_instruction_trace;

/**
 * Clears the ring buffer.
 */
void haeun_trace_clear(haeun_instruction_trace *trace);

/**
 * Records one retired or attempted step (NULL-safe no-op).
 */
void haeun_trace_record(haeun_instruction_trace *trace, uint32_t pc,
                        uint32_t insn, uint8_t op, uint32_t next_pc);

/**
 * Returns the number of recorded entries (at most HAEUN_TRACE_CAPACITY).
 */
uint32_t haeun_trace_size(const haeun_instruction_trace *trace);

/**
 * Copies entry index (0 = oldest retained) into *out. Returns false if OOR.
 */
bool haeun_trace_at(const haeun_instruction_trace *trace, uint32_t index,
                    haeun_trace_entry *out);

#endif
