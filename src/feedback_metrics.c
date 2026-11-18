#include "haeun/feedback.h"

#include "haeun/eval_fairness.h"
#include "haeun/eval_metrics.h"

#include <stdio.h>
#include <string.h>

static void set_metric(haeun_fb_metric *m, const char *name,
                       haeun_fb_validity v, double val, const char *note) {
    memset(m, 0, sizeof(*m));
    snprintf(m->name, sizeof(m->name), "%s", name);
    m->validity = v;
    m->value = val;
    if (note != NULL)
        snprintf(m->note, sizeof(m->note), "%s", note);
}

static void from_opt(haeun_fb_metric *m, const char *name,
                     const haeun_eval_opt *opt) {
    if (opt == NULL || !opt->available)
        set_metric(m, name, HAEUN_FB_UNAVAILABLE, 0.0,
                   opt ? opt->reason : "missing");
    else
        set_metric(m, name, HAEUN_FB_VALID, opt->value, "ok");
}

static uint32_t max_ready(const haeun_eval_run_metrics *m, int *ok) {
    unsigned i;
    uint32_t mx = 0u;
    *ok = 0;
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        if (!m->slot_included[i])
            continue;
        if (!*ok || m->slot_ready[i] > mx)
            mx = m->slot_ready[i];
        *ok = 1;
    }
    return mx;
}

/**
 * Builds a feedback sample from R2.2 run metrics.
 */
haeun_fb_result haeun_fb_sample_from_metrics(const haeun_eval_run_metrics *m,
                                             uint32_t policy_id,
                                             uint64_t timer_count,
                                             const char *workload_id,
                                             uint32_t fingerprint,
                                             haeun_fb_sample *out) {
    haeun_eval_fairness fair;
    int mx_ok = 0;
    uint32_t mx;
    if (m == NULL || out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->policy_id = policy_id;
    out->timer_count = timer_count;
    out->fingerprint = fingerprint;
    if (workload_id != NULL)
        snprintf(out->workload_id, sizeof(out->workload_id), "%s", workload_id);
    if (!m->ok) {
        out->ok = 0;
        set_metric(&out->total_run, "total_run", HAEUN_FB_INVALID, 0.0,
                   m->fail_reason);
        return HAEUN_FB_OK;
    }
    out->ok = 1;
    out->participants = m->participants;
    set_metric(&out->total_run, "total_run", HAEUN_FB_VALID,
               (double)m->total_run_ticks, "guest ticks");
    from_opt(&out->avg_ready_wait, "avg_ready_wait", &m->avg_ready_wait);
    mx = max_ready(m, &mx_ok);
    if (mx_ok)
        set_metric(&out->max_ready_wait, "max_ready_wait", HAEUN_FB_VALID,
                   (double)mx, "max slot ready_wait");
    else
        set_metric(&out->max_ready_wait, "max_ready_wait",
                   HAEUN_FB_INSUFFICIENT_SAMPLE, 0.0, "no participants");
    if (haeun_eval_fairness_from_metrics(m, &fair) != HAEUN_EVAL_OK ||
        !fair.jain_run_ticks.available)
        set_metric(&out->jain_fairness, "jain_fairness", HAEUN_FB_UNAVAILABLE,
                   0.0, fair.jain_run_ticks.reason);
    else
        set_metric(&out->jain_fairness, "jain_fairness", HAEUN_FB_VALID,
                   fair.jain_run_ticks.value, fair.notes);
    set_metric(&out->dispatch_count, "dispatch_count", HAEUN_FB_VALID,
               (double)m->total_dispatch, "sum dispatch_count");
    set_metric(&out->ctx_switches, "ctx_switches", HAEUN_FB_VALID,
               (double)m->context_switches, "host-visible switches");
    set_metric(&out->timer_irqs, "timer_irqs", HAEUN_FB_VALID,
               (double)m->timer_irqs, "timer IRQ count");
    set_metric(&out->completed, "completed", HAEUN_FB_VALID, (double)m->completed,
               "complete_valid slots");
    if (m->completed == 0u)
        set_metric(&out->completion_latency, "completion_latency",
                   HAEUN_FB_UNAVAILABLE, 0.0, "no completions");
    else {
        unsigned i;
        uint64_t sum = 0u;
        unsigned n = 0u;
        for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
            if (!m->slot_included[i] || !m->slot_complete_valid[i])
                continue;
            sum += m->slot_complete_lo[i];
            n++;
        }
        if (n == 0u)
            set_metric(&out->completion_latency, "completion_latency",
                       HAEUN_FB_UNAVAILABLE, 0.0, "no valid stamps");
        else
            set_metric(&out->completion_latency, "completion_latency",
                       HAEUN_FB_VALID, (double)sum / (double)n,
                       "mean complete_lo (32-bit; wrap unknown)");
    }
    return HAEUN_FB_OK;
}

/**
 * Samples live VM telemetry into a feedback sample.
 */
haeun_fb_result haeun_fb_sample_from_vm(const haeun_vm *vm,
                                        const char *workload_id,
                                        uint32_t fingerprint,
                                        haeun_fb_sample *out) {
    haeun_eval_run_metrics m;
    uint32_t pol = 0u;
    if (vm == NULL || vm->ram == NULL || out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (haeun_eval_metrics_from_vm(vm, 0u, 0u, &m) != HAEUN_EVAL_OK)
        return HAEUN_FB_BAD_ARGUMENT;
    if (vm->ram_size > 0xA478u)
        pol = (uint32_t)vm->ram[0xA474] | ((uint32_t)vm->ram[0xA475] << 8) |
              ((uint32_t)vm->ram[0xA476] << 16) |
              ((uint32_t)vm->ram[0xA477] << 24);
    return haeun_fb_sample_from_metrics(&m, pol, vm->timer_count, workload_id,
                                        fingerprint, out);
}
