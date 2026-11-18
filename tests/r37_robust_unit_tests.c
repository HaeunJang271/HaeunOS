/**
 * R3.7 robustness unit tests (host-only).
 */
#include "haeun/robust.h"
#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int test_preset_seed_hash(void) {
    haeun_rb_config cfg;
    haeun_rb_preset p;
    uint32_t s0, s1, h0, h1;
    printf("RUN preset_seed_hash\n");
    memset(&cfg, 0, sizeof(cfg));
    CHECK(haeun_rb_preset_parse("smoke", &p) == 0);
    CHECK(haeun_rb_preset_apply(&cfg, HAEUN_RB_PRESET_SMOKE) == HAEUN_RB_OK);
    CHECK(cfg.reps == 3u);
    CHECK(haeun_rb_preset_apply(&cfg, HAEUN_RB_PRESET_STANDARD) == HAEUN_RB_OK);
    CHECK(cfg.reps == 10u);
    s0 = haeun_rb_workload_seed(HAEUN_WL_SHORT_JOB, 0u);
    s1 = haeun_rb_workload_seed(HAEUN_WL_SHORT_JOB, 0u);
    CHECK(s0 == s1 && s0 != 0u);
    CHECK(haeun_rb_workload_seed(HAEUN_WL_SHORT_JOB, 1u) != s0);
    snprintf(cfg.build_dir, sizeof(cfg.build_dir), "build");
    cfg.profile_id = HAEUN_OPT_PROF_BALANCED;
    h0 = haeun_rb_config_hash(&cfg);
    cfg.reps = 11u;
    h1 = haeun_rb_config_hash(&cfg);
    CHECK(h0 != h1);
    printf("preset_seed_hash ok\n");
    return 0;
}

static int test_compare_and_reliability(void) {
    haeun_rb_run_fp a, b;
    haeun_opt_candidate c;
    printf("RUN compare_and_reliability\n");
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.ok = b.ok = 1;
    a.total_dispatch = b.total_dispatch = 10;
    a.total_run = b.total_run = 100;
    a.total_ready = b.total_ready = 50;
    a.completed = b.completed = 1;
    a.exit_code = b.exit_code = 22;
    CHECK(haeun_rb_compare_fp(&a, &b) == HAEUN_RB_EXACT_MATCH);
    b.exit_code = 23;
    CHECK(haeun_rb_compare_fp(&a, &b) == HAEUN_RB_METRIC_MATCH);
    b.total_run = 999;
    CHECK(haeun_rb_compare_fp(&a, &b) == HAEUN_RB_DIVERGENT);
    a.ok = 0;
    CHECK(haeun_rb_compare_fp(&a, &b) == HAEUN_RB_INVALID);
    memset(&c, 0, sizeof(c));
    c.status = HAEUN_OPT_CAND_VALID;
    c.ready.n = 3;
    c.jain.n = 3;
    c.ready.mean.available = 1;
    CHECK(haeun_rb_classify_reliability(&c, 2u) == HAEUN_RB_REL_SUFFICIENT);
    c.ready.n = 1;
    CHECK(haeun_rb_classify_reliability(&c, 2u) == HAEUN_RB_REL_INSUFFICIENT);
    printf("compare_and_reliability ok\n");
    return 0;
}

static int test_state_schema(void) {
    haeun_rb_state st, st2;
    FILE *fp;
    char path[] = "build/r37_state_test.json";
    printf("RUN state_schema\n");
    haeun_rb_state_init(&st);
    st.overall = HAEUN_RB_OVERALL_WARN;
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    CHECK(haeun_rb_state_save(fp, &st) == HAEUN_RB_OK);
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_rb_state_load(fp, &st2) == HAEUN_RB_OK);
    (void)fclose(fp);
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    fprintf(fp, "{\"schema_version\":99}\n");
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_rb_state_load(fp, &st2) == HAEUN_RB_BAD_STATE);
    (void)fclose(fp);
    printf("state_schema ok\n");
    return 0;
}

/**
 * Entry for R3.7 unit tests.
 */
int main(void) {
    if (test_preset_seed_hash() || test_compare_and_reliability() ||
        test_state_schema())
        return 1;
    printf("r37 robust unit tests passed\n");
    return 0;
}
