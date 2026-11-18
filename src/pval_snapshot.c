#include "haeun/pval_snapshot.h"

#include <stdio.h>
#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_CURR = 0x0000A400u,
    IDLE_FLAG = 0x0000A460u,
    T_IRQ = 0x0000A464u,
    T_CTX = 0x0000A468u,
    POL_ID = 0x0000A474u,
    READY_SEQ_G = 0x0000A46Cu,
    READY_SEQ_WRAP = 0x0000A478u,
    SEAL_END = 0x0000A47Cu,
    OFF_STATE = 0u,
    OFF_PID = 4u,
    OFF_PPID = 8u,
    OFF_EPC = 12u,
    OFF_PTBR = 28u,
    OFF_RUN = 128u,
    OFF_READY_WAIT = 132u,
    OFF_SLEEP = 136u,
    OFF_BLOCKED = 140u,
    OFF_DISPATCH = 144u,
    OFF_COMPLETE_LO = 148u,
    OFF_COMPLETE_VALID = 152u,
    OFF_READY_SINCE = 160u,
    OFF_READY_SEQ_LO = 172u,
    OFF_READY_SEQ_HI = 176u,
    ST_FREE = 0u,
    ST_READY = 1u,
    ST_RUNNING = 2u
};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int ram_ok(const haeun_vm *vm, uint32_t off) {
    return vm != NULL && vm->ram != NULL && (size_t)off + 4u <= vm->ram_size;
}

static uint32_t load(const haeun_vm *vm, uint32_t off) {
    return rd32(vm->ram + off);
}

static void compute_seal(const haeun_vm *vm, uint32_t *seal) {
    uint32_t i;
    uint32_t acc[HAEUN_PVAL_SEAL_WORDS];
    memset(acc, 0, sizeof(acc));
    for (i = PCB_BASE; i < SEAL_END; i += 4u) {
        uint32_t w = load(vm, i);
        uint32_t idx = (i / 4u) % HAEUN_PVAL_SEAL_WORDS;
        acc[idx] ^= w + (i * 0x9e3779b9u);
    }
    acc[0] ^= (uint32_t)vm->timer_count;
    acc[1] ^= (uint32_t)(vm->timer_count >> 32);
    acc[2] ^= (uint32_t)vm->timer_compare;
    acc[3] ^= (uint32_t)(vm->timer_compare >> 32);
    acc[4] ^= vm->timer_control;
    acc[5] ^= vm->csr_ptbr;
    acc[6] ^= vm->pc;
    acc[7] ^= load(vm, POL_ID);
    memcpy(seal, acc, sizeof(acc));
}

/**
 * Reads a const snapshot from the VM. Does not write guest RAM or step the VM.
 */
haeun_pval_result haeun_pval_snapshot_read(const haeun_vm *vm,
                                           haeun_pval_snapshot *out) {
    unsigned i;
    if (vm == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    if (!ram_ok(vm, PCB_BASE) || !ram_ok(vm, POL_ID))
        return HAEUN_PVAL_NO_PCB;
    memset(out, 0, sizeof(*out));
    out->policy_id = load(vm, POL_ID);
    out->curr_index = load(vm, PCB_CURR);
    out->idle_flag = load(vm, IDLE_FLAG);
    out->timer_irqs = load(vm, T_IRQ);
    out->context_switches = load(vm, T_CTX);
    out->timer_count = vm->timer_count;
    out->timer_compare = vm->timer_compare;
    out->timer_control = vm->timer_control;
    out->csr_ptbr = vm->csr_ptbr;
    out->host_pc = vm->pc;
    out->host_steps_marker = vm->timer_count; /* proxy: no separate step counter */
    for (i = 0; i < HAEUN_PVAL_NPROC; ++i) {
        uint32_t base = PCB_BASE + (uint32_t)i * PCB_STRIDE;
        haeun_pval_slot *s = &out->slots[i];
        s->slot = (uint32_t)i;
        s->state = load(vm, base + OFF_STATE);
        s->pid = load(vm, base + OFF_PID);
        s->ppid = load(vm, base + OFF_PPID);
        s->epc = load(vm, base + OFF_EPC);
        s->ptbr = load(vm, base + OFF_PTBR);
        s->run_ticks = load(vm, base + OFF_RUN);
        s->ready_wait_ticks = load(vm, base + OFF_READY_WAIT);
        s->sleep_ticks = load(vm, base + OFF_SLEEP);
        s->blocked_ticks = load(vm, base + OFF_BLOCKED);
        s->dispatch_count = load(vm, base + OFF_DISPATCH);
        s->complete_lo = load(vm, base + OFF_COMPLETE_LO);
        s->complete_valid = load(vm, base + OFF_COMPLETE_VALID);
        s->ready_since = load(vm, base + OFF_READY_SINCE);
        s->ready_seq_lo = load(vm, base + OFF_READY_SEQ_LO);
        s->ready_seq_hi = load(vm, base + OFF_READY_SEQ_HI);
        s->as_slot_valid = (i < HAEUN_PVAL_AS_SLOTS) ? 1 : 0;
        if (s->state == ST_FREE && s->pid == 0u && s->complete_valid == 0u) {
            s->is_participant = 0;
            s->telem_kind = (s->run_ticks || s->dispatch_count)
                                ? HAEUN_PVAL_FIELD_STALE
                                : HAEUN_PVAL_FIELD_NA;
        } else if (!s->as_slot_valid) {
            s->is_participant = 0;
            s->telem_kind = HAEUN_PVAL_FIELD_NA;
        } else {
            s->is_participant = 1;
            s->telem_kind = HAEUN_PVAL_FIELD_VALID;
            out->participant_count++;
            if (s->state == ST_READY || s->state == ST_RUNNING)
                out->ready_eligible_count++;
        }
    }
    out->ready_seq_global_lo = load(vm, READY_SEQ_G);
    out->ready_seq_global_hi = load(vm, READY_SEQ_G + 4u);
    out->ready_seq_wrap = load(vm, READY_SEQ_WRAP);
    compute_seal(vm, out->seal);
    out->seal_valid = 1;
    fprintf(stderr, "haeun-pval: snapshot policy=%u curr=%u eligible=%u\n",
            out->policy_id, out->curr_index, out->ready_eligible_count);
    return HAEUN_PVAL_OK;
}

/**
 * Recomputes the seal from the live VM and compares to snap->seal.
 */
haeun_pval_result haeun_pval_snapshot_verify_unchanged(const haeun_vm *vm,
                                                       const haeun_pval_snapshot *snap) {
    uint32_t seal[HAEUN_PVAL_SEAL_WORDS];
    if (vm == NULL || snap == NULL || !snap->seal_valid)
        return HAEUN_PVAL_BAD_ARGUMENT;
    if (!ram_ok(vm, POL_ID))
        return HAEUN_PVAL_NO_PCB;
    compute_seal(vm, seal);
    if (memcmp(seal, snap->seal, sizeof(seal)) != 0) {
        fprintf(stderr, "haeun-pval: guest mutation detected during validation\n");
        return HAEUN_PVAL_MUTATION;
    }
    if (vm->timer_count != snap->timer_count ||
        vm->timer_compare != snap->timer_compare ||
        vm->timer_control != snap->timer_control ||
        vm->csr_ptbr != snap->csr_ptbr || vm->pc != snap->host_pc)
        return HAEUN_PVAL_MUTATION;
    return HAEUN_PVAL_OK;
}
