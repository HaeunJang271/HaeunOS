#include "haeun/optimize.h"

#include <stdio.h>

static const char *dec_name(haeun_opt_decision d) {
    switch (d) {
    case HAEUN_OPT_DEC_SELECT:
        return "SELECT";
    case HAEUN_OPT_DEC_NO_DECISION:
        return "NO_DECISION";
    case HAEUN_OPT_DEC_EQUIVALENT:
        return "EQUIVALENT";
    default:
        return "INCONCLUSIVE";
    }
}

/**
 * Human-readable optimization report.
 */
void haeun_opt_fprint(FILE *fp, const haeun_opt_state *st) {
    unsigned i, c;
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp, "HaeunOS Autonomous Policy Optimization (R3.6)\n");
    fprintf(fp, "schema: %u  controller: %s  mode: %d  profile: %s  reps: %u\n",
            st->schema_version, st->controller_version, (int)st->mode,
            st->profile.name, st->cfg.reps);
    fprintf(fp, "rationale: %s\n", st->profile.rationale);
    fprintf(fp, "disclaimer: deterministic optimization; not ML; guest ticks≠wall-clock\n");
    for (i = 0; i < st->wl_count; ++i) {
        const haeun_opt_wl_result *w = &st->workloads[i];
        fprintf(fp, "\nworkload=%s decision=%s policy=%u\n  %s\n",
                w->workload_id, dec_name(w->decision), w->recommended_policy,
                w->reason);
        for (c = 0; c < w->cand_count; ++c) {
            const haeun_opt_candidate *k = &w->cands[c];
            fprintf(fp,
                    "  cand=%s status=%d ok=%u fail=%u pareto=%d score_valid=%d",
                    k->name, (int)k->status, k->success, k->fail,
                    k->pareto_nondominated, k->score_valid);
            if (k->score_valid)
                fprintf(fp, " score=%.4f", k->score);
            fprintf(fp, "\n");
            if (k->ready.mean.available)
                fprintf(fp, "    ready_mean=%.4f n=%u\n", k->ready.mean.value,
                        k->ready.n);
            if (k->jain.mean.available)
                fprintf(fp, "    jain_mean=%.4f n=%u\n", k->jain.mean.value,
                        k->jain.n);
        }
    }
    if (st->last_rec.valid)
        fprintf(fp, "\nlast_rec: %s policy=%u gen=%u\n",
                st->last_rec.experiment_id, st->last_rec.recommended_policy,
                st->last_rec.generation);
}

/**
 * JSON report with schema_version.
 */
void haeun_opt_fprint_json(FILE *fp, const haeun_opt_state *st) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":%u,\"controller_version\":\"%s\",\"mode\":%d,"
            "\"profile\":\"%s\",\"reps\":%u,\"wl_count\":%u,\"workloads\":[",
            st->schema_version, st->controller_version, (int)st->mode,
            st->profile.name, st->cfg.reps, st->wl_count);
    for (i = 0; i < st->wl_count; ++i) {
        const haeun_opt_wl_result *w = &st->workloads[i];
        fprintf(fp,
                "%s{\"workload_id\":\"%s\",\"decision\":\"%s\","
                "\"recommended_policy\":%u,\"reason\":\"%s\","
                "\"cands\":[{\"name\":\"%s\",\"status\":%d,\"pareto\":%d},"
                "{\"name\":\"%s\",\"status\":%d,\"pareto\":%d}]}",
                i ? "," : "", w->workload_id, dec_name(w->decision),
                w->recommended_policy, w->reason, w->cands[0].name,
                (int)w->cands[0].status, w->cands[0].pareto_nondominated,
                w->cands[1].name, (int)w->cands[1].status,
                w->cands[1].pareto_nondominated);
    }
    fprintf(fp,
            "],\"last_rec\":{\"valid\":%s,\"policy\":%u,\"decision\":\"%s\","
            "\"generation\":%u},\"limitations\":\"experimental; not statistical "
            "proof\"}\n",
            st->last_rec.valid ? "true" : "false",
            st->last_rec.recommended_policy, dec_name(st->last_rec.decision),
            st->last_rec.generation);
}
