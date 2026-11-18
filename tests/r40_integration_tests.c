/**
 * R4.0 integration smoke: reuse R3.7 robust pipeline + host overhead sample.
 * Usage: r40_integration_tests <build-dir>
 */
#include "haeun/robust.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * Runs integrated smoke campaign and exports build/r40 reports.
 */
int main(int argc, char **argv) {
    haeun_rb_state st;
    clock_t t0, t1;
    double sec;
    unsigned i;
    if (argc != 2) {
        fprintf(stderr, "usage: r40_integration_tests <build-dir>\n");
        return 2;
    }
    printf("=== R4.0 integration smoke ===\n");
    haeun_rb_state_init(&st);
    snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", argv[1]);
    snprintf(st.cfg.build_id, sizeof(st.cfg.build_id), "r40-smoke");
    CHECK(haeun_rb_preset_apply(&st.cfg, HAEUN_RB_PRESET_SMOKE) == HAEUN_RB_OK);
    t0 = clock();
    CHECK(haeun_rb_run_experiments(&st) == HAEUN_RB_OK);
    t1 = clock();
    sec = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    CHECK(haeun_rb_run_sensitivity(&st) == HAEUN_RB_OK);
    CHECK(haeun_rb_workload_robustness(&st) == HAEUN_RB_OK);
    CHECK(haeun_rb_verify_recommendations(&st) == HAEUN_RB_OK);
    (void)haeun_rb_reconfig_stress(&st, NULL, 1);
    haeun_rb_aggregate(&st);
    CHECK(st.wl_count == (unsigned)HAEUN_WL_COUNT);
    CHECK(st.overall != HAEUN_RB_OVERALL_FAIL);
    for (i = 0; i < st.wl_count; ++i) {
        CHECK(st.workloads[i].replay == HAEUN_RB_EXACT_MATCH ||
              st.workloads[i].replay == HAEUN_RB_METRIC_MATCH);
    }
    CHECK(haeun_rb_export(&st, "build/r40") == HAEUN_RB_OK);
    printf("host_tooling_wall_s=%.3f (clock(); research-tool overhead)\n", sec);
    printf("overall=%d wl=%u\n", (int)st.overall, st.wl_count);
    haeun_rb_fprint(stdout, &st);
    printf("r40 integration smoke passed\n");
    return 0;
}
