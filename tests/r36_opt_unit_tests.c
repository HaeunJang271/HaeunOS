/**
 * R3.6 optimizer unit tests (host-only; no guest).
 */
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

static int test_profiles_and_norm(void) {
    haeun_opt_profile p;
    haeun_opt_profile_id id;
    double na, nb;
    printf("RUN profiles_and_norm\n");
    CHECK(haeun_opt_profile_get(HAEUN_OPT_PROF_BALANCED, &p) == HAEUN_OPT_OK);
    CHECK(p.obj_count >= 2u);
    CHECK(haeun_opt_profile_parse("fairness", &id) == 0);
    CHECK(id == HAEUN_OPT_PROF_FAIRNESS);
    CHECK(haeun_opt_profile_parse("nope", &id) != 0);
    CHECK(haeun_opt_normalize_pair(10.0, 20.0, HAEUN_OPT_DIR_MIN, &na, &nb));
    CHECK(na > nb);
    CHECK(haeun_opt_normalize_pair(10.0, 20.0, HAEUN_OPT_DIR_MAX, &na, &nb));
    CHECK(nb > na);
    printf("profiles_and_norm ok\n");
    return 0;
}

static int test_stats_ci(void) {
    double v[] = {1.0, 2.0, 3.0, 4.0};
    haeun_opt_metric_stats s;
    printf("RUN stats_ci\n");
    CHECK(haeun_opt_stats_fill(v, 4u, 1u, "avg_ready_wait", &s) == HAEUN_OPT_OK);
    CHECK(s.n == 4u && s.missing == 1u);
    CHECK(s.mean.available && s.ci95_lo.available && s.ci95_hi.available);
    CHECK(haeun_opt_stats_fill(v, 1u, 0u, "x", &s) == HAEUN_OPT_OK);
    CHECK(!s.ci95_lo.available);
    printf("stats_ci ok\n");
    return 0;
}

static void plant_cand(haeun_opt_candidate *c, uint32_t pol, double ready,
                       double jain, double disp) {
    memset(c, 0, sizeof(*c));
    c->policy_id = pol;
    snprintf(c->name, sizeof(c->name), "%s", pol ? "FIFO" : "RR");
    c->status = HAEUN_OPT_CAND_VALID;
    c->success = 3u;
    c->ready.n = 3u;
    c->ready.mean.available = 1;
    c->ready.mean.value = ready;
    c->jain.n = 3u;
    c->jain.mean.available = 1;
    c->jain.mean.value = jain;
    c->dispatch.n = 3u;
    c->dispatch.mean.available = 1;
    c->dispatch.mean.value = disp;
    c->completed.n = 3u;
    c->completed.mean.available = 1;
    c->completed.mean.value = 1.0;
}

static int test_pareto_and_select(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN pareto_and_select\n");
    CHECK(haeun_opt_profile_get(HAEUN_OPT_PROF_LATENCY, &p) == HAEUN_OPT_OK);
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_cand(&wl.cands[0], 0u, 100.0, 0.9, 50.0);
    plant_cand(&wl.cands[1], 1u, 40.0, 0.8, 40.0); /* better ready */
    haeun_opt_pareto_mark(wl.cands, 2u);
    CHECK(wl.cands[0].pareto_nondominated == 1 ||
          wl.cands[1].pareto_nondominated == 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    CHECK(wl.decision == HAEUN_OPT_DEC_SELECT);
    CHECK(wl.recommended_policy == 1u);
    /* near-equal → EQUIVALENT */
    plant_cand(&wl.cands[0], 0u, 50.0, 0.9, 40.0);
    plant_cand(&wl.cands[1], 1u, 51.0, 0.9, 40.0);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    CHECK(wl.decision == HAEUN_OPT_DEC_EQUIVALENT ||
          wl.decision == HAEUN_OPT_DEC_SELECT);
    printf("pareto_and_select ok\n");
    return 0;
}

static int test_state_schema(void) {
    haeun_opt_state st, st2;
    FILE *fp;
    char path[] = "build/r36_state_test.json";
    printf("RUN state_schema\n");
    haeun_opt_state_init(&st);
    st.cycle_id = 7u;
    st.last_rec.valid = 1;
    st.last_rec.decision = HAEUN_OPT_DEC_SELECT;
    st.last_rec.recommended_policy = 1u;
    st.last_rec.generation = 7u;
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    CHECK(haeun_opt_state_save(fp, &st) == HAEUN_OPT_OK);
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_opt_state_load(fp, &st2) == HAEUN_OPT_OK);
    (void)fclose(fp);
    CHECK(st2.cycle_id == 7u);
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    fprintf(fp, "{\"schema_version\":99}\n");
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_opt_state_load(fp, &st2) == HAEUN_OPT_BAD_STATE);
    (void)fclose(fp);
    printf("state_schema ok\n");
    return 0;
}

/**
 * Entry for R3.6 optimizer unit tests.
 */
int main(void) {
    if (test_profiles_and_norm() || test_stats_ci() || test_pareto_and_select() ||
        test_state_schema())
        return 1;
    printf("r36 opt unit tests passed\n");
    return 0;
}
