#ifndef HAEUN_POLICY_VERIFY_H
#define HAEUN_POLICY_VERIFY_H

#include "haeun/vm.h"

#include <stddef.h>
#include <stdint.h>

enum {
    HAEUN_POLICY_RR = 0,
    HAEUN_POLICY_FCFS = 1,
    HAEUN_POLICY_SLOTS = 3,
    HAEUN_POLICY_NPROC = 4,
    HAEUN_SCHED_TRACE_CAP = 64
};

/** Host-side mirror of guest policy_select (no guest execution). */
typedef struct haeun_policy_fixture {
    uint32_t policy_id;
    uint32_t curr;
    uint32_t state[HAEUN_POLICY_NPROC];
    /** ready_since per slot; legacy FCFS mirror fallback. */
    uint32_t ready_since[HAEUN_POLICY_NPROC];
    /** R3.4 ready_seq; if any nonzero, FCFS mirror prefers these. */
    uint32_t ready_seq_lo[HAEUN_POLICY_NPROC];
    uint32_t ready_seq_hi[HAEUN_POLICY_NPROC];
} haeun_policy_fixture;

/**
 * Runs the host policy mirror (RR or FCFS). Sets *out_slot on success.
 * Returns 1 on success, 0 on no-candidate / invalid policy ID.
 */
int haeun_policy_select_host(const haeun_policy_fixture *fx, uint32_t *out_slot);

/** One host-observed schedule-related sample (test harness only). */
typedef struct haeun_sched_trace_event {
    uint32_t event_index;
    uint32_t current_slot_before;
    uint32_t policy_id;
    uint32_t selection_success;
    uint32_t selected_slot;
    uint32_t current_slot_after;
    uint32_t selected_state;
    uint32_t dispatch_count;
    uint32_t context_switches;
} haeun_sched_trace_event;

typedef struct haeun_sched_trace {
    unsigned count;
    uint32_t last_curr;
    uint32_t last_disp0;
    int primed;
    haeun_sched_trace_event events[HAEUN_SCHED_TRACE_CAP];
} haeun_sched_trace;

/**
 * Clears a host-side scheduler trace buffer.
 */
void haeun_sched_trace_reset(haeun_sched_trace *tr);

/**
 * Observes guest RAM after a VM step. Appends an event when curr or
 * PCB0 dispatch_count changes. Never writes guest RAM.
 */
void haeun_sched_trace_observe(haeun_sched_trace *tr, const haeun_vm *vm);

#endif
