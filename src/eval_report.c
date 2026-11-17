#include "haeun/eval_report.h"

#include "haeun/eval_fairness.h"
#include "haeun/eval_stats.h"

#include <string.h>

static void emit_opt_json(FILE *fp, const char *key, const haeun_eval_opt *o) {
    if (o->available)
        fprintf(fp, "\"%s\":%.6f", key, o->value);
    else
        fprintf(fp, "\"%s\":null,\"%s_unavailable\":\"%s\"", key, key,
                o->reason);
}

static void emit_stat_json(FILE *fp, const char *name,
                           const haeun_eval_stat_summary *s) {
    fprintf(fp, "\"%s\":{\"sample_count\":%u,", name, s->sample_count);
    emit_opt_json(fp, "mean", &s->mean);
    fputc(',', fp);
    emit_opt_json(fp, "median", &s->median);
    fputc(',', fp);
    emit_opt_json(fp, "min", &s->min);
    fputc(',', fp);
    emit_opt_json(fp, "max", &s->max);
    fputc(',', fp);
    emit_opt_json(fp, "stddev", &s->stddev);
    fputc('}', fp);
}

/**
 * Fills aggregate stats from successful runs; optional cost if weights != NULL.
 */
haeun_eval_result haeun_eval_report_prepare(const haeun_eval_experiment *exp,
                                            const haeun_eval_cost_weights *weights,
                                            haeun_eval_report_bundle *out) {
    double runs[HAEUN_EVAL_MAX_RUNS];
    double ready[HAEUN_EVAL_MAX_RUNS];
    double disp[HAEUN_EVAL_MAX_RUNS];
    double ctx[HAEUN_EVAL_MAX_RUNS];
    unsigned n = 0u;
    unsigned i;
    if (exp == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->exp = exp;
    for (i = 0; i < exp->requested_reps; ++i) {
        const haeun_eval_run_metrics *m = &exp->runs[i].metrics;
        if (!m->ok)
            continue;
        runs[n] = (double)m->total_run_ticks;
        ready[n] = (double)m->total_ready_wait;
        disp[n] = (double)m->total_dispatch;
        ctx[n] = (double)m->context_switches;
        n++;
    }
    (void)haeun_eval_stats_compute(runs, n, &out->st_run);
    (void)haeun_eval_stats_compute(ready, n, &out->st_ready);
    (void)haeun_eval_stats_compute(disp, n, &out->st_dispatch);
    (void)haeun_eval_stats_compute(ctx, n, &out->st_ctx);
    for (i = 0; i < exp->requested_reps; ++i) {
        if (!exp->runs[i].metrics.ok)
            continue;
        if (haeun_eval_fairness_from_metrics(&exp->runs[i].metrics,
                                             &out->fairness_first_ok) ==
            HAEUN_EVAL_OK)
            out->fairness_available = 1;
        break;
    }
    if (weights != NULL) {
        for (i = 0; i < exp->requested_reps; ++i) {
            if (!exp->runs[i].metrics.ok)
                continue;
            if (haeun_eval_cost_compute(&exp->runs[i].metrics, weights,
                                        &out->cost) == HAEUN_EVAL_OK)
                out->cost_available = 1;
            break;
        }
    }
    return HAEUN_EVAL_OK;
}

/**
 * Prints a human-readable evaluation report.
 */
void haeun_eval_report_fprint(FILE *fp, const haeun_eval_report_bundle *b) {
    const haeun_eval_experiment *e;
    if (fp == NULL || b == NULL || b->exp == NULL)
        return;
    e = b->exp;
    fprintf(fp, "HaeunOS Scheduler Evaluation (R2.2/R3.0)\n");
    fprintf(fp, "experiment_id: %s\n", e->experiment_id);
    fprintf(fp, "build_id: %s\n", e->build_id);
    fprintf(fp, "policy: %s id=%u\n", e->policy_name, e->policy_id);
    fprintf(fp, "workload: %s category=%s image=%s seed=%u rng=%d\n",
            e->workload.name, e->workload.category, e->workload.image_name,
            e->workload.seed, e->workload.uses_rng);
    fprintf(fp, "reps: requested=%u success=%u failed=%u\n", e->requested_reps,
            e->success_count, e->fail_count);
    fprintf(fp, "aggregate total_run_ticks mean=");
    if (b->st_run.mean.available)
        fprintf(fp, "%.1f\n", b->st_run.mean.value);
    else
        fprintf(fp, "unavailable (%s)\n", b->st_run.mean.reason);
    fprintf(fp, "aggregate ready_wait mean=");
    if (b->st_ready.mean.available)
        fprintf(fp, "%.1f\n", b->st_ready.mean.value);
    else
        fprintf(fp, "unavailable (%s)\n", b->st_ready.mean.reason);
    if (b->fairness_available) {
        fprintf(fp, "fairness Jain(run_ticks)=");
        if (b->fairness_first_ok.jain_run_ticks.available)
            fprintf(fp, "%.6f n=%u min=%u max=%u\n",
                    b->fairness_first_ok.jain_run_ticks.value,
                    b->fairness_first_ok.n, b->fairness_first_ok.min_run,
                    b->fairness_first_ok.max_run);
        else
            fprintf(fp, "unavailable\n");
        fprintf(fp, "fairness note: %s\n", b->fairness_first_ok.notes);
    }
    if (b->cost_available) {
        fprintf(fp, "cost_function experimental=");
        if (b->cost.score.available)
            fprintf(fp, "%.6f (not a real-world prediction)\n",
                    b->cost.score.value);
        else
            fprintf(fp, "unavailable (%s)\n", b->cost.score.reason);
    }
    fprintf(fp, "limitations: guest ticks != wall clock; FREE/slot3 excluded; "
                "32-bit modular counters; ready_wait not exact across policy "
                "instruction growth.\n");
}

/**
 * Prints schema-versioned JSON suitable for later automation.
 */
void haeun_eval_report_fprint_json(FILE *fp, const haeun_eval_report_bundle *b) {
    const haeun_eval_experiment *e;
    unsigned i;
    if (fp == NULL || b == NULL || b->exp == NULL)
        return;
    e = b->exp;
    fprintf(fp, "{\"schema_version\":%d,\"build_id\":\"%s\",", HAEUN_EVAL_SCHEMA,
            e->build_id);
    fprintf(fp,
            "\"experiment\":{\"id\":\"%s\",\"policy_id\":%u,\"policy_name\":\"%s\","
            "\"requested_reps\":%u,\"success_count\":%u,\"fail_count\":%u},",
            e->experiment_id, e->policy_id, e->policy_name, e->requested_reps,
            e->success_count, e->fail_count);
    fprintf(fp,
            "\"workload\":{\"name\":\"%s\",\"category\":\"%s\","
            "\"image\":\"%s\",\"seed\":%u,\"uses_rng\":%s,"
            "\"expected_exit\":%u,\"step_cap\":%llu},",
            e->workload.name, e->workload.category, e->workload.image_name,
            e->workload.seed, e->workload.uses_rng ? "true" : "false",
            e->workload.expected_exit,
            (unsigned long long)e->workload.step_cap);
    fprintf(fp, "\"runs\":[");
    for (i = 0; i < e->requested_reps; ++i) {
        const haeun_eval_run_metrics *m = &e->runs[i].metrics;
        if (i)
            fputc(',', fp);
        fprintf(fp, "{\"index\":%u,\"ok\":%s,", i, m->ok ? "true" : "false");
        if (!m->ok)
            fprintf(fp, "\"fail_reason\":\"%s\"}", m->fail_reason);
        else
            fprintf(fp,
                    "\"exit_code\":%u,\"steps\":%llu,\"participants\":%u,"
                    "\"completed\":%u,\"incomplete\":%u,"
                    "\"total_run_ticks\":%llu,\"total_ready_wait\":%llu,"
                    "\"total_sleep\":%llu,\"total_blocked\":%llu,"
                    "\"total_dispatch\":%llu,\"context_switches\":%u,"
                    "\"timer_irqs\":%u}",
                    m->exit_code, (unsigned long long)m->steps, m->participants,
                    m->completed, m->incomplete,
                    (unsigned long long)m->total_run_ticks,
                    (unsigned long long)m->total_ready_wait,
                    (unsigned long long)m->total_sleep,
                    (unsigned long long)m->total_blocked,
                    (unsigned long long)m->total_dispatch, m->context_switches,
                    m->timer_irqs);
    }
    fprintf(fp, "],\"aggregates\":{");
    emit_stat_json(fp, "total_run_ticks", &b->st_run);
    fputc(',', fp);
    emit_stat_json(fp, "total_ready_wait", &b->st_ready);
    fputc(',', fp);
    emit_stat_json(fp, "total_dispatch", &b->st_dispatch);
    fputc(',', fp);
    emit_stat_json(fp, "context_switches", &b->st_ctx);
    fprintf(fp, "},\"disclaimer\":\"experimental evaluation; not a performance "
                "claim\"}");
}
