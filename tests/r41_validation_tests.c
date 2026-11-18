/**
 * R4.1 research validation: smoke then optional standard campaign.
 * Usage: r41_validation_tests <build-dir> [smoke|standard|both]
 * Default: both (smoke first; standard only if smoke passes).
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

static int run_campaign(const char *build, haeun_rb_preset preset,
                        const char *build_id, const char *out_dir) {
    haeun_rb_state st;
    clock_t t0, t1;
    double sec;
    unsigned i;
    haeun_rb_state_init(&st);
    snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build);
    snprintf(st.cfg.build_id, sizeof(st.cfg.build_id), "%s", build_id);
    snprintf(st.cfg.source_revision, sizeof(st.cfg.source_revision), "%s",
             "r41-local");
    CHECK(haeun_rb_preset_apply(&st.cfg, preset) == HAEUN_RB_OK);
    printf("=== R4.1 campaign preset=%d reps=%u out=%s ===\n", (int)preset,
           st.cfg.reps, out_dir);
    printf("estimate_baseline_runs=%u (wl=%d * policies=2 * reps)\n",
           (unsigned)HAEUN_WL_COUNT * 2u * st.cfg.reps, (int)HAEUN_WL_COUNT);
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
    CHECK(haeun_rb_export(&st, out_dir) == HAEUN_RB_OK);
    printf("host_tooling_wall_s=%.3f config_hash=0x%08x overall=%d\n", sec,
           (unsigned)st.cfg.config_hash, (int)st.overall);
    haeun_rb_fprint(stdout, &st);
    return 0;
}

/**
 * Entry: smoke mandatory; standard if argv says so or both.
 */
int main(int argc, char **argv) {
    const char *mode = "both";
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: r41_validation_tests <build-dir> "
                        "[smoke|standard|both]\n");
        return 2;
    }
    if (argc == 3)
        mode = argv[2];
    printf("=== R4.1 research validation ===\n");
    if (strcmp(mode, "standard") != 0) {
        if (run_campaign(argv[1], HAEUN_RB_PRESET_SMOKE, "r41-smoke",
                         "build/r41-smoke") != 0)
            return 1;
        printf("r41 smoke passed\n");
    }
    if (strcmp(mode, "smoke") != 0) {
        /* Resource decision: smoke ~sub-second host clock; standard ~3.3x
         * baseline matrix (120 runs) is feasible on this host. */
        printf("resource_decision=RUN_STANDARD "
               "(smoke_host_clock_observed_prior; baseline=120 runs)\n");
        if (run_campaign(argv[1], HAEUN_RB_PRESET_STANDARD, "r41-standard",
                         "build/r41-standard") != 0)
            return 1;
        printf("r41 standard passed\n");
    }
    printf("r41 validation tests passed\n");
    return 0;
}
