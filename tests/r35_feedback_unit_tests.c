/**
 * R3.5 feedback metric/decision/state unit tests (host-only).
 */
#include "haeun/feedback.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void plant_metrics(haeun_eval_run_metrics *m, uint32_t run0,
                          uint32_t run1, uint32_t ready0, uint32_t ready1,
                          uint64_t dispatch) {
    memset(m, 0, sizeof(*m));
    m->ok = 1;
    m->participants = 2u;
    m->completed = 1u;
    m->total_run_ticks = (uint64_t)run0 + run1;
    m->total_ready_wait = (uint64_t)ready0 + ready1;
    m->total_dispatch = dispatch;
    m->context_switches = 4u;
    m->timer_irqs = 2u;
    m->slot_included[0] = 1;
    m->slot_included[1] = 1;
    m->slot_run[0] = run0;
    m->slot_run[1] = run1;
    m->slot_ready[0] = ready0;
    m->slot_ready[1] = ready1;
    m->slot_complete_valid[0] = 1;
    m->slot_complete_lo[0] = 100u;
    m->avg_ready_wait.available = 1;
    m->avg_ready_wait.value = (double)(ready0 + ready1) / 2.0;
}

static int test_metrics_and_fairness(void) {
    haeun_eval_run_metrics m;
    haeun_fb_sample s;
    printf("RUN metrics_and_fairness\n");
    plant_metrics(&m, 100u, 100u, 50u, 50u, 10u);
    CHECK(haeun_fb_sample_from_metrics(&m, 0u, 1u, "cpu_bound", 1u, &s) ==
          HAEUN_FB_OK);
    CHECK(s.ok && s.jain_fairness.validity == HAEUN_FB_VALID);
    CHECK(s.avg_ready_wait.validity == HAEUN_FB_VALID);
    CHECK(s.completion_latency.validity == HAEUN_FB_VALID);
    memset(&m, 0, sizeof(m));
    m.ok = 0;
    snprintf(m.fail_reason, sizeof(m.fail_reason), "broken");
    CHECK(haeun_fb_sample_from_metrics(&m, 0u, 1u, "x", 1u, &s) == HAEUN_FB_OK);
    CHECK(!s.ok && s.total_run.validity == HAEUN_FB_INVALID);
    printf("metrics_and_fairness ok\n");
    return 0;
}

static int test_compare_and_outcomes(void) {
    haeun_eval_run_metrics mb, ma;
    haeun_fb_sample b, a;
    haeun_fb_config cfg;
    haeun_fb_assessment as;
    printf("RUN compare_and_outcomes\n");
    haeun_fb_config_default(&cfg);
    plant_metrics(&mb, 100u, 100u, 80u, 80u, 20u);
    plant_metrics(&ma, 100u, 100u, 40u, 40u, 10u);
    CHECK(haeun_fb_sample_from_metrics(&mb, 0u, 1u, "mixed", 7u, &b) ==
          HAEUN_FB_OK);
    CHECK(haeun_fb_sample_from_metrics(&ma, 1u, 2u, "mixed", 7u, &a) ==
          HAEUN_FB_OK);
    CHECK(haeun_fb_evaluate(&cfg, &b, &a, &as) == HAEUN_FB_OK);
    CHECK(as.comparable && as.outcome == HAEUN_FB_IMPROVED);
    /* fingerprint mismatch → inconclusive/incomparable */
    a.fingerprint = 99u;
    CHECK(haeun_fb_evaluate(&cfg, &b, &a, &as) == HAEUN_FB_INCOMPARABLE);
    CHECK(as.outcome == HAEUN_FB_INCONCLUSIVE);
    /* degrade ready wait */
    plant_metrics(&ma, 100u, 100u, 200u, 200u, 40u);
    CHECK(haeun_fb_sample_from_metrics(&ma, 1u, 2u, "mixed", 7u, &a) ==
          HAEUN_FB_OK);
    CHECK(haeun_fb_evaluate(&cfg, &b, &a, &as) == HAEUN_FB_OK);
    CHECK(as.outcome == HAEUN_FB_DEGRADED || as.outcome == HAEUN_FB_INCONCLUSIVE);
    printf("compare_and_outcomes ok\n");
    return 0;
}

static int test_gates_and_state(void) {
    haeun_fb_state st;
    haeun_plan plan;
    haeun_fb_assessment a;
    FILE *fp;
    char path[] = "build/r35_state_test.json";
    haeun_fb_state st2;
    printf("RUN gates_and_state\n");
    haeun_fb_state_init(&st);
    memset(&plan, 0, sizeof(plan));
    plan.decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    plan.source_policy = 0u;
    plan.target_policy = 1u;
    st.cooldown_remaining = 2u;
    haeun_fb_apply_to_plan(&st, &plan);
    CHECK(plan.decision == HAEUN_PLAN_KEEP_CURRENT);
    plan.decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    plan.target_policy = 1u;
    st.cooldown_remaining = 0u;
    st.previous_outcome_valid = 1;
    st.previous_outcome = HAEUN_FB_INCONCLUSIVE;
    haeun_fb_apply_to_plan(&st, &plan);
    CHECK(plan.decision == HAEUN_PLAN_NO_DECISION);
    memset(&a, 0, sizeof(a));
    a.outcome = HAEUN_FB_DEGRADED;
    snprintf(a.reason, sizeof(a.reason), "test");
    plan.decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    haeun_fb_record_outcome(&st, &plan, &a, 1);
    CHECK(st.consecutive_regressions >= 1u);
    CHECK(st.cooldown_remaining == st.cfg.cooldown_cycles);
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    CHECK(haeun_fb_state_save(fp, &st) == HAEUN_FB_OK);
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_fb_state_load(fp, &st2) == HAEUN_FB_OK);
    (void)fclose(fp);
    CHECK(st2.schema_version == HAEUN_FB_SCHEMA);
    CHECK(st2.cycle_id == st.cycle_id);
    /* bad schema */
    fp = fopen(path, "wb");
    CHECK(fp != NULL);
    fprintf(fp, "{\"schema_version\":99}\n");
    (void)fclose(fp);
    fp = fopen(path, "rb");
    CHECK(fp != NULL);
    CHECK(haeun_fb_state_load(fp, &st2) == HAEUN_FB_BAD_STATE);
    (void)fclose(fp);
    printf("gates_and_state ok\n");
    return 0;
}

static int test_shadow_no_commit_flag(void) {
    haeun_fb_state st;
    haeun_plan plan;
    printf("RUN shadow_no_commit_flag\n");
    haeun_fb_state_init(&st);
    CHECK(st.mode == HAEUN_FB_MODE_SHADOW);
    memset(&plan, 0, sizeof(plan));
    plan.decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    plan.source_policy = 0;
    plan.target_policy = 1;
    /* Without VM, only gates; ensure apply does not set txn flags. */
    haeun_fb_apply_to_plan(&st, &plan);
    CHECK(plan.txn_committed == 0);
    printf("shadow_no_commit_flag ok\n");
    return 0;
}

/**
 * Entry for R3.5 feedback unit tests.
 */
int main(void) {
    if (test_metrics_and_fairness() || test_compare_and_outcomes() ||
        test_gates_and_state() || test_shadow_no_commit_flag())
        return 1;
    printf("r35 feedback unit tests passed\n");
    return 0;
}
