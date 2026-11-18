#include "haeun/policy_verify.h"

#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_CURR = 0x0000A400u,
    POL_ID = 0x0000A474u,
    T_CTX = 0x0000A468u,
    OFF_STATE = 0u,
    OFF_DISPATCH = 144u,
    ST_READY = 1u,
    ST_RUNNING = 2u
};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/** Returns 1 if a is modular-older than b (signed(a-b) < 0). */
static int older_u32(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) < 0;
}

static int select_rr(const haeun_policy_fixture *fx, uint32_t *out_slot) {
    uint32_t slot = fx->curr;
    uint32_t scanned;
    for (scanned = 0u; scanned < HAEUN_POLICY_SLOTS; ++scanned) {
        slot += 1u;
        if (slot >= HAEUN_POLICY_SLOTS)
            slot = 0u;
        if (slot < HAEUN_POLICY_NPROC) {
            uint32_t st = fx->state[slot];
            if (st == ST_READY || st == ST_RUNNING) {
                *out_slot = slot;
                return 1;
            }
        }
    }
    return 0;
}

static int select_fcfs(const haeun_policy_fixture *fx, uint32_t *out_slot) {
    uint32_t slot;
    int have = 0;
    int use_seq = 0;
    uint32_t best = 0u;
    uint32_t best_rs = 0u;
    uint64_t best_seq = 0u;
    for (slot = 0u; slot < HAEUN_POLICY_SLOTS; ++slot) {
        if (fx->ready_seq_lo[slot] != 0u || fx->ready_seq_hi[slot] != 0u)
            use_seq = 1;
    }
    for (slot = 0u; slot < HAEUN_POLICY_SLOTS; ++slot) {
        uint32_t st = fx->state[slot];
        if (st != ST_READY && st != ST_RUNNING)
            continue;
        if (use_seq) {
            uint64_t sq = ((uint64_t)fx->ready_seq_hi[slot] << 32) |
                          (uint64_t)fx->ready_seq_lo[slot];
            if (!have || sq < best_seq || (sq == best_seq && slot < best)) {
                have = 1;
                best = slot;
                best_seq = sq;
            }
        } else {
            uint32_t rs = fx->ready_since[slot];
            if (!have || older_u32(rs, best_rs) ||
                (rs == best_rs && slot < best)) {
                have = 1;
                best = slot;
                best_rs = rs;
            }
        }
    }
    if (!have)
        return 0;
    *out_slot = best;
    return 1;
}

/**
 * Host mirror of guest policy_select / policy_rr / policy_fcfs.
 */
int haeun_policy_select_host(const haeun_policy_fixture *fx, uint32_t *out_slot) {
    if (fx == NULL || out_slot == NULL)
        return 0;
    if (fx->policy_id == HAEUN_POLICY_RR)
        return select_rr(fx, out_slot);
    if (fx->policy_id == HAEUN_POLICY_FCFS)
        return select_fcfs(fx, out_slot);
    return 0;
}

/**
 * Clears a host-side scheduler trace buffer.
 */
void haeun_sched_trace_reset(haeun_sched_trace *tr) {
    if (tr == NULL)
        return;
    memset(tr, 0, sizeof(*tr));
}

/**
 * Records a deterministic host-side event when curr or dispatch_count moves.
 */
void haeun_sched_trace_observe(haeun_sched_trace *tr, const haeun_vm *vm) {
    uint32_t curr;
    uint32_t disp;
    uint32_t pol;
    uint32_t ctx;
    uint32_t st;
    haeun_sched_trace_event *ev;
    if (tr == NULL || vm == NULL || vm->ram == NULL)
        return;
    if (vm->ram_size < (size_t)(POL_ID + 4u))
        return;
    curr = rd32(vm->ram + PCB_CURR);
    disp = rd32(vm->ram + PCB_BASE + OFF_DISPATCH);
    pol = rd32(vm->ram + POL_ID);
    ctx = rd32(vm->ram + T_CTX);
    if (!tr->primed) {
        tr->last_curr = curr;
        tr->last_disp0 = disp;
        tr->primed = 1;
        return;
    }
    if (curr == tr->last_curr && disp == tr->last_disp0)
        return;
    if (tr->count >= HAEUN_SCHED_TRACE_CAP) {
        tr->last_curr = curr;
        tr->last_disp0 = disp;
        return;
    }
    ev = &tr->events[tr->count];
    ev->event_index = tr->count;
    ev->current_slot_before = tr->last_curr;
    ev->policy_id = pol;
    ev->selection_success = 1u;
    ev->selected_slot = curr;
    ev->current_slot_after = curr;
    st = 0u;
    if (curr < HAEUN_POLICY_NPROC)
        st = rd32(vm->ram + PCB_BASE + curr * PCB_STRIDE + OFF_STATE);
    ev->selected_state = st;
    ev->dispatch_count = disp;
    ev->context_switches = ctx;
    tr->count++;
    tr->last_curr = curr;
    tr->last_disp0 = disp;
}
