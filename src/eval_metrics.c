#include "haeun/eval_metrics.h"

#include <stdio.h>
#include <string.h>

enum { STATE_FREE = 0, STATE_ZOMBIE = 5 };

static void opt_set(haeun_eval_opt *o, double v) {
    o->available = 1;
    o->value = v;
    o->reason[0] = '\0';
}

static void opt_na(haeun_eval_opt *o, const char *why) {
    o->available = 0;
    o->value = 0.0;
    snprintf(o->reason, sizeof(o->reason), "%s", why);
}

static int include_slot(const haeun_telem_slot *s) {
    if (!s->as_slot_valid)
        return 0;
    if (s->state == STATE_FREE && s->complete_valid == 0u &&
        s->dispatch_count == 0u && s->run_ticks == 0u)
        return 0;
    if (s->state == STATE_FREE && s->complete_valid == 0u &&
        s->pid == 0u && s->dispatch_count == 0u)
        return 0;
    /* Never treat raw FREE stale-only slots as participants. */
    if (s->state == STATE_FREE && s->pid == 0u && s->complete_valid == 0u)
        return 0;
    return 1;
}

/**
 * Builds run metrics from a telemetry snapshot.
 */
haeun_eval_result haeun_eval_metrics_from_telem(const haeun_telem_report *rep,
                                                uint32_t exit_code,
                                                uint64_t steps,
                                                haeun_eval_run_metrics *out) {
    unsigned i;
    if (rep == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->ok = 1;
    out->exit_code = exit_code;
    out->steps = steps;
    out->context_switches = rep->context_switches;
    out->timer_irqs = rep->timer_irqs;
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        const haeun_telem_slot *s = &rep->slots[i];
        out->slot_run[i] = s->run_ticks;
        out->slot_ready[i] = s->ready_wait_ticks;
        out->slot_complete_valid[i] = s->complete_valid;
        out->slot_complete_lo[i] = s->complete_lo;
        out->slot_pid[i] = s->pid;
        if (!include_slot(s))
            continue;
        out->slot_included[i] = 1;
        out->participants++;
        out->total_run_ticks += s->run_ticks;
        out->total_ready_wait += s->ready_wait_ticks;
        out->total_sleep += s->sleep_ticks;
        out->total_blocked += s->blocked_ticks;
        out->total_dispatch += s->dispatch_count;
        if (s->complete_valid != 0u || s->state == STATE_ZOMBIE)
            out->completed++;
        else
            out->incomplete++;
    }
    if (out->participants == 0u)
        opt_na(&out->avg_ready_wait, "no participating processes");
    else
        opt_set(&out->avg_ready_wait,
                (double)out->total_ready_wait / (double)out->participants);
    if (out->total_dispatch == 0u)
        opt_na(&out->ctx_dispatch_ratio, "dispatch_count is zero");
    else
        opt_set(&out->ctx_dispatch_ratio,
                (double)out->context_switches / (double)out->total_dispatch);
    return HAEUN_EVAL_OK;
}

/**
 * Reads guest telemetry then fills metrics. Does not write guest RAM.
 */
haeun_eval_result haeun_eval_metrics_from_vm(const haeun_vm *vm, uint32_t exit_code,
                                             uint64_t steps,
                                             haeun_eval_run_metrics *out) {
    haeun_telem_report rep;
    if (haeun_telem_read(vm, &rep) != HAEUN_TELEM_OK) {
        fprintf(stderr, "haeun-eval: telem_read failed\n");
        return HAEUN_EVAL_IO;
    }
    return haeun_eval_metrics_from_telem(&rep, exit_code, steps, out);
}
