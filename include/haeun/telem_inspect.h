#ifndef HAEUN_TELEM_INSPECT_H
#define HAEUN_TELEM_INSPECT_H

#include "haeun/vm.h"

#include <stdio.h>
#include <stdint.h>

enum {
    HAEUN_TELEM_NPROC = 4,
    HAEUN_TELEM_NOTE_MAX = 8,
    HAEUN_TELEM_NOTE_LEN = 96
};

typedef enum {
    HAEUN_TELEM_OK = 0,
    HAEUN_TELEM_BAD_ARGUMENT,
    HAEUN_TELEM_NO_PCB_PAGE
} haeun_telem_result;

typedef enum {
    HAEUN_TELEM_NOTE_INFO = 0,
    HAEUN_TELEM_NOTE_EXPECTED = 1,
    HAEUN_TELEM_NOTE_WARN = 2
} haeun_telem_note_kind;

/** One PCB slot's raw R1.2 counters plus identity fields (host-side view). */
typedef struct haeun_telem_slot {
    uint32_t slot;
    uint32_t state;
    uint32_t pid;
    uint32_t run_ticks;
    uint32_t ready_wait_ticks;
    uint32_t sleep_ticks;
    uint32_t blocked_ticks;
    uint32_t dispatch_count;
    uint32_t complete_lo;
    uint32_t complete_valid;
    uint32_t account_lo;
    uint32_t ready_since;
    uint32_t sleep_since;
    uint32_t blocked_since;
    /** 0 when the slot has no process address space (slot 3). */
    int as_slot_valid;
} haeun_telem_slot;

/** Read-only snapshot of process-v1 global and per-PCB telemetry. */
typedef struct haeun_telem_report {
    uint32_t idle_flag;
    uint32_t timer_irqs;
    uint32_t context_switches;
    uint32_t curr_index;
    haeun_telem_slot slots[HAEUN_TELEM_NPROC];
    unsigned note_count;
    haeun_telem_note_kind note_kind[HAEUN_TELEM_NOTE_MAX];
    char notes[HAEUN_TELEM_NOTE_MAX][HAEUN_TELEM_NOTE_LEN];
} haeun_telem_report;

/**
 * Fills a report by reading guest RAM only. Does not write RAM or CSRs.
 */
haeun_telem_result haeun_telem_read(const haeun_vm *vm, haeun_telem_report *out);

/**
 * Returns a static name for PCB state 0..5, or "UNKNOWN".
 */
const char *haeun_telem_state_name(uint32_t state);

/**
 * Prints a human-readable telemetry report to fp.
 */
void haeun_telem_fprint(FILE *fp, const haeun_telem_report *rep);

/**
 * Prints a compact JSON object for machine consumption.
 */
void haeun_telem_fprint_json(FILE *fp, const haeun_telem_report *rep);

#endif
