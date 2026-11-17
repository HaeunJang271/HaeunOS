#include "haeun/eval_compare.h"

#include <string.h>

/**
 * Compares two experiment bundles under identical workloads.
 * Policies may differ (RR vs FCFS); no superiority claims.
 */
haeun_eval_result haeun_eval_compare(const haeun_eval_report_bundle *a,
                                     const haeun_eval_report_bundle *b,
                                     FILE *fp) {
    const haeun_eval_experiment *ea;
    const haeun_eval_experiment *eb;
    if (a == NULL || b == NULL || a->exp == NULL || b->exp == NULL || fp == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    ea = a->exp;
    eb = b->exp;
    fprintf(fp, "Comparison (observational; no policy ranking)\n");
    fprintf(fp, "A: %s workload=%s policy=%s(%u)\n", ea->experiment_id,
            ea->workload.name, ea->policy_name, ea->policy_id);
    fprintf(fp, "B: %s workload=%s policy=%s(%u)\n", eb->experiment_id,
            eb->workload.name, eb->policy_name, eb->policy_id);
    if (strcmp(ea->workload.name, eb->workload.name) != 0 ||
        ea->workload.kind != eb->workload.kind) {
        fprintf(fp, "incomparable: workload definitions differ\n");
        return HAEUN_EVAL_INCOMPARABLE;
    }
    if (ea->workload.seed != eb->workload.seed ||
        ea->workload.step_cap != eb->workload.step_cap) {
        fprintf(fp, "incomparable: seed or step_cap differ\n");
        return HAEUN_EVAL_INCOMPARABLE;
    }
    if (ea->success_count == 0u || eb->success_count == 0u) {
        fprintf(fp, "incomparable: one side has zero successful runs\n");
        return HAEUN_EVAL_INCOMPARABLE;
    }
    fprintf(fp, "success A=%u B=%u failed A=%u B=%u\n", ea->success_count,
            eb->success_count, ea->fail_count, eb->fail_count);
    if (a->st_run.mean.available && b->st_run.mean.available)
        fprintf(fp, "delta mean total_run_ticks: %.1f\n",
                b->st_run.mean.value - a->st_run.mean.value);
    else
        fprintf(fp, "mean total_run_ticks: missing on one side\n");
    if (a->st_ready.mean.available && b->st_ready.mean.available)
        fprintf(fp, "delta mean ready_wait: %.1f\n",
                b->st_ready.mean.value - a->st_ready.mean.value);
    else
        fprintf(fp, "mean ready_wait: missing on one side\n");
    if (a->st_ctx.mean.available && b->st_ctx.mean.available)
        fprintf(fp, "delta mean context_switches: %.1f\n",
                b->st_ctx.mean.value - a->st_ctx.mean.value);
    else
        fprintf(fp, "mean context_switches: missing on one side\n");
    fprintf(fp, "note: deltas are observational; not superiority.\n");
    return HAEUN_EVAL_OK;
}
