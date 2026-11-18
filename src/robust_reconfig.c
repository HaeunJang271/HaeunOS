#include "haeun/robust.h"

#include "haeun/reconfig.h"

#include <stdio.h>
#include <string.h>

enum { POL = 0x0000A474u };

static uint32_t rd32(const uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off] | ((uint32_t)ram[off + 1u] << 8) |
           ((uint32_t)ram[off + 2u] << 16) | ((uint32_t)ram[off + 3u] << 24);
}

static int try_switch(haeun_vm *vm, uint32_t src, uint32_t dst) {
    haeun_recfg_txn txn;
    haeun_recfg_result rr;
    haeun_recfg_fi_clear();
    rr = haeun_recfg_request(&txn, vm, src, dst);
    if (rr != HAEUN_RECFG_OK)
        return 0;
    rr = haeun_recfg_prepare(&txn, vm);
    if (rr != HAEUN_RECFG_OK)
        return 0;
    rr = haeun_recfg_commit(&txn, vm);
    if (rr == HAEUN_RECFG_FAILED) {
        (void)haeun_recfg_rollback(&txn, vm);
        return 0;
    }
    return rr == HAEUN_RECFG_OK && txn.state == HAEUN_RECFG_ST_COMMITTED;
}

/**
 * Reconfiguration stress using R3.2 on a live VM.
 * dry_run: count planned ops only; no commits.
 */
haeun_rb_result haeun_rb_reconfig_stress(haeun_rb_state *st, haeun_vm *vm,
                                         int dry_run) {
    uint32_t cur;
    unsigned i;
    if (st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    st->stress_transitions = 0u;
    st->stress_failures = 0u;
    if (dry_run || vm == NULL || vm->ram == NULL) {
        st->stress_transitions = 0u;
        return HAEUN_RB_OK;
    }
    cur = rd32(vm->ram, POL);
    /* Invalid target must fail. */
    if (try_switch(vm, cur, 99u))
        st->stress_failures++;
    else
        st->stress_transitions++; /* counted as handled rejection */
    cur = rd32(vm->ram, POL);
    for (i = 0; i < 4u; ++i) {
        uint32_t dst = (cur == 0u) ? 1u : 0u;
        if (try_switch(vm, cur, dst)) {
            st->stress_transitions++;
            cur = rd32(vm->ram, POL);
            if (cur != dst)
                st->stress_failures++;
        } else {
            /* UNKNOWN/FAIL reject is acceptable; record failure of commit. */
            st->stress_failures++;
            cur = rd32(vm->ram, POL);
        }
    }
    /* Injected FI failure path. */
    {
        haeun_recfg_txn txn;
        haeun_recfg_fi_clear();
        haeun_recfg_fi_arm(HAEUN_RECFG_FI_BEFORE_COMMIT);
        if (haeun_recfg_request(&txn, vm, cur, cur == 0u ? 1u : 0u) ==
            HAEUN_RECFG_OK) {
            if (haeun_recfg_prepare(&txn, vm) == HAEUN_RECFG_OK) {
                if (haeun_recfg_commit(&txn, vm) == HAEUN_RECFG_OK)
                    st->stress_failures++; /* should not succeed */
                else
                    st->stress_transitions++;
            }
        }
        haeun_recfg_fi_clear();
    }
    return HAEUN_RB_OK;
}
