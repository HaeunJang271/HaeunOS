#include "haeun/instruction_trace.h"

/**
 * Resets the instruction trace ring.
 */
void haeun_trace_clear(haeun_instruction_trace *trace) {
    uint32_t i;
    if (trace == NULL) return;
    for (i = 0u; i < HAEUN_TRACE_CAPACITY; ++i) {
        trace->entries[i].pc = 0u;
        trace->entries[i].insn = 0u;
        trace->entries[i].next_pc = 0u;
        trace->entries[i].op = 0u;
    }
    trace->head = 0u;
    trace->count = 0u;
}

/**
 * Appends one step record, dropping the oldest when full.
 */
void haeun_trace_record(haeun_instruction_trace *trace, uint32_t pc,
                        uint32_t insn, uint8_t op, uint32_t next_pc) {
    haeun_trace_entry *slot;
    if (trace == NULL) return;
    slot = &trace->entries[trace->head];
    slot->pc = pc;
    slot->insn = insn;
    slot->op = op;
    slot->next_pc = next_pc;
    trace->head = (trace->head + 1u) % HAEUN_TRACE_CAPACITY;
    if (trace->count < HAEUN_TRACE_CAPACITY) trace->count += 1u;
}

/**
 * Returns how many entries are retained.
 */
uint32_t haeun_trace_size(const haeun_instruction_trace *trace) {
    return trace == NULL ? 0u : trace->count;
}

/**
 * Reads a retained entry by oldest-first index.
 */
bool haeun_trace_at(const haeun_instruction_trace *trace, uint32_t index,
                    haeun_trace_entry *out) {
    uint32_t start;
    if (trace == NULL || out == NULL || index >= trace->count) return false;
    start = (trace->head + HAEUN_TRACE_CAPACITY - trace->count) %
            HAEUN_TRACE_CAPACITY;
    *out = trace->entries[(start + index) % HAEUN_TRACE_CAPACITY];
    return true;
}
