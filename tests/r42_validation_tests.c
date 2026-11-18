/**
 * R4.2: multi-profile recommendation matrix + performance stats (n=10).
 * Runs each workload×{RR,FIFO} once, then re-scores under all 5 profiles.
 * Usage: r42_validation_tests <build-dir>
 */
#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static const char *prof_name(haeun_opt_profile_id id) {
    static const char *n[] = {"balanced", "fairness", "latency", "throughput",
                              "low_overhead"};
    return id < HAEUN_OPT_PROF_COUNT ? n[id] : "?";
}

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

static void emit_opt(FILE *fp, const haeun_eval_opt *o) {
    if (o->available)
        fprintf(fp, "%.6g", o->value);
    else
        fputs("null", fp);
}

/**
 * Writes R4.2 profile matrix + performance JSON under research/experiments/.
 */
int main(int argc, char **argv) {
    haeun_opt_config cfg;
    haeun_opt_wl_result base[HAEUN_WL_COUNT];
    unsigned k, p, planned = 0u, ok_runs = 0u, fail_runs = 0u;
    FILE *fp_prof, *fp_perf, *fp_stat;
    clock_t t0, t1;
    if (argc != 2) {
        fprintf(stderr, "usage: r42_validation_tests <build-dir>\n");
        return 2;
    }
    printf("=== R4.2 multi-profile + performance (fresh run) ===\n");
#ifdef _WIN32
    (void)_mkdir("research");
    (void)_mkdir("research/experiments");
#else
    (void)mkdir("research", 0755);
    (void)mkdir("research/experiments", 0755);
#endif
    haeun_opt_config_default(&cfg);
    snprintf(cfg.build_dir, sizeof(cfg.build_dir), "%s", argv[1]);
    snprintf(cfg.software_version, sizeof(cfg.software_version), "%s",
             "r42-local");
    cfg.reps = 10u;
    cfg.min_success = 10u;
    cfg.experimental = 1;
    t0 = clock();
    for (k = 0; k < HAEUN_WL_COUNT; ++k) {
        haeun_opt_profile bal;
        CHECK(haeun_opt_profile_get(HAEUN_OPT_PROF_BALANCED, &bal) ==
              HAEUN_OPT_OK);
        CHECK(haeun_opt_run_workload(&cfg, &bal, (haeun_eval_wl_kind)k,
                                     &base[k]) == HAEUN_OPT_OK);
        CHECK(base[k].cand_count == 2u);
        planned += cfg.reps * 2u;
        ok_runs += base[k].cands[0].success + base[k].cands[1].success;
        fail_runs += base[k].cands[0].fail + base[k].cands[1].fail;
    }
    t1 = clock();
    fp_prof = fopen("research/experiments/r42-profile-verification.json", "wb");
    fp_perf = fopen("research/experiments/r42-performance-results.json", "wb");
    fp_stat = fopen("research/experiments/r42-statistics.json", "wb");
    CHECK(fp_prof && fp_perf && fp_stat);
    fprintf(fp_prof,
            "{\"campaign\":\"r42-profile\",\"reps\":10,\"build_dir\":\"%s\","
            "\"note\":\"experiments once then re-score per profile\","
            "\"cells\":[",
            argv[1]);
    fprintf(fp_perf,
            "{\"campaign\":\"r42-performance\",\"planned_runs\":%u,"
            "\"success_runs\":%u,\"fail_runs\":%u,\"host_clock_s\":%.3f,"
            "\"workloads\":[",
            planned, ok_runs, fail_runs,
            (double)(t1 - t0) / (double)CLOCKS_PER_SEC);
    fprintf(fp_stat,
            "{\"design\":\"independent_fresh_vm_per_eval_run\","
            "\"paired\":false,\"n_per_cell\":10,"
            "\"inferential_tests\":\"NOT_PERFORMED\","
            "\"reason\":\"assumptions_not_validated_for_significance\","
            "\"workloads\":[");
    for (k = 0; k < HAEUN_WL_COUNT; ++k) {
        haeun_opt_candidate *rr = &base[k].cands[0];
        haeun_opt_candidate *fi = &base[k].cands[1];
        if (k)
            fputc(',', fp_perf), fputc(',', fp_stat);
        fprintf(fp_perf,
                "{\"workload\":\"%s\",\"RR\":{\"success\":%u,\"fail\":%u,"
                "\"ready_mean\":",
                base[k].workload_id, rr->success, rr->fail);
        emit_opt(fp_perf, &rr->ready.mean);
        fprintf(fp_perf, ",\"jain_mean\":");
        emit_opt(fp_perf, &rr->jain.mean);
        fprintf(fp_perf, ",\"dispatch_mean\":");
        emit_opt(fp_perf, &rr->dispatch.mean);
        fprintf(fp_perf,
                "},\"FIFO\":{\"success\":%u,\"fail\":%u,\"ready_mean\":",
                fi->success, fi->fail);
        emit_opt(fp_perf, &fi->ready.mean);
        fprintf(fp_perf, ",\"jain_mean\":");
        emit_opt(fp_perf, &fi->jain.mean);
        fprintf(fp_perf, ",\"dispatch_mean\":");
        emit_opt(fp_perf, &fi->dispatch.mean);
        fprintf(fp_perf, "}}");
        fprintf(fp_stat,
                "{\"workload\":\"%s\",\"RR\":{\"n\":%u,\"ready\":{\"mean\":",
                base[k].workload_id, rr->ready.n);
        emit_opt(fp_stat, &rr->ready.mean);
        fprintf(fp_stat, ",\"median\":");
        emit_opt(fp_stat, &rr->ready.median);
        fprintf(fp_stat, ",\"stddev\":");
        emit_opt(fp_stat, &rr->ready.stddev);
        fprintf(fp_stat, ",\"min\":");
        emit_opt(fp_stat, &rr->ready.min);
        fprintf(fp_stat, ",\"max\":");
        emit_opt(fp_stat, &rr->ready.max);
        fprintf(fp_stat, ",\"ci95_lo\":");
        emit_opt(fp_stat, &rr->ready.ci95_lo);
        fprintf(fp_stat, ",\"ci95_hi\":");
        emit_opt(fp_stat, &rr->ready.ci95_hi);
        fprintf(fp_stat, "},\"jain\":{\"mean\":");
        emit_opt(fp_stat, &rr->jain.mean);
        fprintf(fp_stat, ",\"stddev\":");
        emit_opt(fp_stat, &rr->jain.stddev);
        fprintf(fp_stat, "}},\"FIFO\":{\"n\":%u,\"ready\":{\"mean\":",
                fi->ready.n);
        emit_opt(fp_stat, &fi->ready.mean);
        fprintf(fp_stat, ",\"median\":");
        emit_opt(fp_stat, &fi->ready.median);
        fprintf(fp_stat, ",\"stddev\":");
        emit_opt(fp_stat, &fi->ready.stddev);
        fprintf(fp_stat, ",\"min\":");
        emit_opt(fp_stat, &fi->ready.min);
        fprintf(fp_stat, ",\"max\":");
        emit_opt(fp_stat, &fi->ready.max);
        fprintf(fp_stat, ",\"ci95_lo\":");
        emit_opt(fp_stat, &fi->ready.ci95_lo);
        fprintf(fp_stat, ",\"ci95_hi\":");
        emit_opt(fp_stat, &fi->ready.ci95_hi);
        fprintf(fp_stat, "},\"jain\":{\"mean\":");
        emit_opt(fp_stat, &fi->jain.mean);
        fprintf(fp_stat, ",\"stddev\":");
        emit_opt(fp_stat, &fi->jain.stddev);
        fprintf(fp_stat, "}}}");
        for (p = 0; p < HAEUN_OPT_PROF_COUNT; ++p) {
            haeun_opt_profile prof;
            haeun_opt_wl_result wl = base[k];
            haeun_opt_wl_result wl2;
            int replay_ok;
            CHECK(haeun_opt_profile_get((haeun_opt_profile_id)p, &prof) ==
                  HAEUN_OPT_OK);
            CHECK(haeun_opt_select(&prof, &wl) == HAEUN_OPT_OK);
            wl2 = wl;
            CHECK(haeun_opt_select(&prof, &wl2) == HAEUN_OPT_OK);
            replay_ok = (wl.decision == wl2.decision &&
                         wl.recommended_policy == wl2.recommended_policy);
            if (k || p)
                fputc(',', fp_prof);
            fprintf(fp_prof,
                    "{\"workload\":\"%s\",\"profile\":\"%s\",\"decision\":\"%s\","
                    "\"recommended_policy\":%u,\"recommended_name\":\"%s\","
                    "\"cand_count\":%u,\"reason\":\"%s\",\"replay_same\":%s,"
                    "\"apply_path\":\"NOT_TESTED_per_cell\","
                    "\"safety_gate\":\"UNKNOWN_neq_PASS_contract\","
                    "\"evidence\":\"OBSERVED\"}",
                    base[k].workload_id, prof_name((haeun_opt_profile_id)p),
                    dec_name(wl.decision), (unsigned)wl.recommended_policy,
                    wl.decision == HAEUN_OPT_DEC_SELECT
                        ? (wl.recommended_policy ? "FIFO" : "RR")
                        : "none",
                    wl.cand_count, wl.reason, replay_ok ? "true" : "false");
            printf("  %s/%s -> %s pol=%u replay=%d\n", base[k].workload_id,
                   prof_name((haeun_opt_profile_id)p), dec_name(wl.decision),
                   (unsigned)wl.recommended_policy, replay_ok);
            CHECK(replay_ok);
        }
    }
    fputs("]}\n", fp_prof);
    fputs("]}\n", fp_perf);
    fputs("]}\n", fp_stat);
    fclose(fp_prof);
    fclose(fp_perf);
    fclose(fp_stat);
    CHECK(planned == 120u);
    CHECK(fail_runs == 0u);
    CHECK(ok_runs == 120u);
    printf("planned=%u ok=%u fail=%u profiles=%d\n", planned, ok_runs,
           fail_runs, (int)HAEUN_OPT_PROF_COUNT);
    printf("r42 validation tests passed\n");
    return 0;
}
