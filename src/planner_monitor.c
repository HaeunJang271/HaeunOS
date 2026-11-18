#include "haeun/planner.h"

#include "haeun/eval_metrics.h"

#include <string.h>

enum { POL_OFF = 0x0000A474u };

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/**
 * Collects telemetry, policy ID, and eval metrics. Read-only on guest RAM.
 */
haeun_plan_result haeun_plan_monitor(const haeun_vm *vm, haeun_plan_obs *out,
                                     uint32_t generation) {
    haeun_telem_result tr;
    haeun_eval_result er;
    if (vm == NULL || out == NULL || vm->ram == NULL)
        return HAEUN_PLAN_BAD_ARGUMENT;
    if (vm->ram_size < (size_t)(POL_OFF + 4u))
        return HAEUN_PLAN_NO_TELEM;
    memset(out, 0, sizeof(*out));
    out->snapshot_generation = generation;
    out->timer_count = vm->timer_count;
    out->policy_id = rd32(vm->ram + POL_OFF);
    out->policy_ok = (out->policy_id == HAEUN_EVAL_POLICY_RR ||
                      out->policy_id == HAEUN_EVAL_POLICY_FCFS);
    tr = haeun_telem_read(vm, &out->telem);
    out->telem_ok = (tr == HAEUN_TELEM_OK);
    if (!out->telem_ok)
        return HAEUN_PLAN_NO_TELEM;
    er = haeun_eval_metrics_from_telem(&out->telem, 0u, 0u, &out->metrics);
    out->metrics_ok = (er == HAEUN_EVAL_OK && out->metrics.ok);
    return HAEUN_PLAN_OK;
}
