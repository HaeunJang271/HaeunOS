/**
 * R2.2 runner tests: execute catalog workloads, JSON report, determinism.
 * Usage: r22_eval_runner_tests <build-dir>
 */
#include "haeun/eval_cost.h"
#include "haeun/eval_report.h"
#include "haeun/eval_runner.h"
#include "haeun/eval_workload.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int run_wl(const char *dir, haeun_eval_wl_kind kind, unsigned reps) {
    haeun_eval_workload wl;
    haeun_eval_experiment exp;
    haeun_eval_report_bundle bundle;
    haeun_eval_cost_weights weights;
    char json[8192];
    FILE *mem;
    size_t n;
    CHECK(haeun_eval_workload_get(kind, &wl) == HAEUN_EVAL_OK);
    printf("RUN workload %s\n", wl.name);
    CHECK(haeun_eval_experiment_run(&wl, dir, HAEUN_EVAL_POLICY_RR, wl.name, reps,
                                    &exp) == HAEUN_EVAL_OK);
    CHECK(exp.fail_count == 0u);
    CHECK(exp.success_count == reps);
    CHECK(exp.runs[0].metrics.ok);
    CHECK(exp.runs[0].metrics.participants >= wl.expected_min_pids);
    if (reps >= 2u) {
        CHECK(exp.runs[0].metrics.total_run_ticks ==
              exp.runs[1].metrics.total_run_ticks);
        CHECK(exp.runs[0].metrics.context_switches ==
              exp.runs[1].metrics.context_switches);
        CHECK(exp.runs[0].metrics.total_dispatch ==
              exp.runs[1].metrics.total_dispatch);
    }
    haeun_eval_cost_default_weights(&weights);
    CHECK(haeun_eval_report_prepare(&exp, &weights, &bundle) == HAEUN_EVAL_OK);
    mem = tmpfile();
    CHECK(mem != NULL);
    haeun_eval_report_fprint(mem, &bundle);
    haeun_eval_report_fprint_json(mem, &bundle);
    rewind(mem);
    n = fread(json, 1u, sizeof(json) - 1u, mem);
    json[n] = '\0';
    (void)fclose(mem);
    CHECK(strstr(json, "\"schema_version\":1") != NULL);
    CHECK(strstr(json, "\"ok\":true") != NULL);
    CHECK(strstr(json, HAEUN_EVAL_BUILD_ID) != NULL);
    printf("%s ok\n", wl.name);
    return 0;
}

/**
 * Runs R2.2 guest evaluation smoke across the workload catalog.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r22_eval_runner_tests <build-dir>\n");
        return 2;
    }
    if (run_wl(argv[1], HAEUN_WL_CPU_BOUND, 2u) ||
        run_wl(argv[1], HAEUN_WL_SHORT_JOB, 2u) ||
        run_wl(argv[1], HAEUN_WL_MIXED, 1u) ||
        run_wl(argv[1], HAEUN_WL_BURSTY, 2u) ||
        run_wl(argv[1], HAEUN_WL_BLOCK_SLEEP, 1u) ||
        run_wl(argv[1], HAEUN_WL_BLOCK_WAIT, 1u))
        return 1;
    printf("r22 eval runner tests passed\n");
    return 0;
}
