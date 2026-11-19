/**
 * R4.7 Gate A: host-only recommendation discrimination tests (S1-S9).
 * Planted inputs only; production optimizer/profiles untouched.
 * Schema analysis labels are distinct from haeun_opt_decision enums.
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

/** Schema analysis label (not a production enum). */
typedef enum {
    R47_LABEL_SELECT_RR = 0,
    R47_LABEL_SELECT_FIFO,
    R47_LABEL_SCORE_EQUIVALENCE,
    R47_LABEL_NO_DECISION,
    R47_LABEL_INCONCLUSIVE,
    R47_LABEL_INSUFFICIENT_TELEMETRY
} r47_analysis_label;

static int nearly_eq(double a, double b) {
    double d = a - b;
    return (d < 0.0 ? -d : d) < 1e-9;
}

/** Maps opt decision to R4.7 schema analysis label. */
static r47_analysis_label map_analysis_label(const haeun_opt_wl_result *wl,
                                             int force_insufficient) {
    if (force_insufficient)
        return R47_LABEL_INSUFFICIENT_TELEMETRY;
    if (wl->decision == HAEUN_OPT_DEC_SELECT) {
        if (wl->recommended_policy == 0u)
            return R47_LABEL_SELECT_RR;
        if (wl->recommended_policy == 1u)
            return R47_LABEL_SELECT_FIFO;
        return R47_LABEL_INCONCLUSIVE;
    }
    if (wl->decision == HAEUN_OPT_DEC_EQUIVALENT)
        return R47_LABEL_SCORE_EQUIVALENCE;
    if (wl->decision == HAEUN_OPT_DEC_NO_DECISION)
        return R47_LABEL_NO_DECISION;
    return R47_LABEL_INCONCLUSIVE;
}

/** Plants VALID latency candidate (ready/dispatch means). */
static void plant_latency(haeun_opt_candidate *c, uint32_t pol, double ready,
                          double disp, int ready_avail, int disp_avail) {
    memset(c, 0, sizeof(*c));
    c->policy_id = pol;
    snprintf(c->name, sizeof(c->name), "%s", pol ? "FIFO" : "RR");
    c->status = HAEUN_OPT_CAND_VALID;
    c->success = 3u;
    c->reps = 3u;
    snprintf(c->ready.name, sizeof(c->ready.name), "avg_ready_wait");
    c->ready.n = ready_avail ? 3u : 0u;
    c->ready.mean.available = ready_avail;
    c->ready.mean.value = ready;
    snprintf(c->dispatch.name, sizeof(c->dispatch.name), "dispatch_count");
    c->dispatch.n = disp_avail ? 3u : 0u;
    c->dispatch.mean.available = disp_avail;
    c->dispatch.mean.value = disp;
}

/** Loads latency profile into a local copy. */
static int load_latency(haeun_opt_profile *p) {
    CHECK(haeun_opt_profile_get(HAEUN_OPT_PROF_LATENCY, p) == HAEUN_OPT_OK);
    CHECK(p->obj_count == 2u);
    CHECK(nearly_eq(p->min_improve, 0.05));
    return 0;
}

static int check_scores(const haeun_opt_wl_result *wl, double rr, double fifo) {
    CHECK(wl->cands[0].score_valid && wl->cands[1].score_valid);
    CHECK(nearly_eq(wl->cands[0].score, rr));
    CHECK(nearly_eq(wl->cands[1].score, fifo));
    return 0;
}

static int case_s1(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S1\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 40.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 100.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.875, 0.125))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_SELECT && wl.recommended_policy == 0u);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SELECT_RR);
    printf("S1 PASS opt=SELECT pol=0 analysis=SELECT_RR scores=0.875/0.125\n");
    return 0;
}

static int case_s2(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S2\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 100.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 40.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.125, 0.875))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_SELECT && wl.recommended_policy == 1u);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SELECT_FIFO);
    printf("S2 PASS opt=SELECT pol=1 analysis=SELECT_FIFO scores=0.125/0.875\n");
    return 0;
}

static int case_s3(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S3\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 50.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 50.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.5, 0.5))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_EQUIVALENT);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SCORE_EQUIVALENCE);
    printf("S3 PASS opt=EQUIVALENT analysis=SCORE_EQUIVALENCE scores=0.5/0.5\n");
    return 0;
}

/**
 * S4: score_valid=0. Latency uses ready+dispatch; ready-only missing still
 * scores dispatch, so both objectives are planted unavailable.
 */
static int case_s4(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S4\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 0.0, 0.0, 0, 0);
    plant_latency(&wl.cands[1], 1u, 0.0, 0.0, 0, 0);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_INCONCLUSIVE);
    CHECK(!wl.cands[0].score_valid && !wl.cands[1].score_valid);
    CHECK(wl.decision == HAEUN_OPT_DEC_INCONCLUSIVE);
    CHECK(map_analysis_label(&wl, 1) == R47_LABEL_INSUFFICIENT_TELEMETRY);
    printf("S4 PASS opt=INCONCLUSIVE analysis=INSUFFICIENT_TELEMETRY "
           "score_valid=0\n");
    return 0;
}

static int case_s5(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S5\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 40.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 100.0, 50.0, 1, 1);
    wl.cands[1].status = HAEUN_OPT_CAND_INVALID;
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    CHECK(wl.decision == HAEUN_OPT_DEC_NO_DECISION);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_NO_DECISION);
    printf("S5 PASS opt=NO_DECISION analysis=NO_DECISION\n");
    return 0;
}

static int case_s6(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S6\n");
    if (load_latency(&p))
        return 1;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 70.0, 30.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 70.0, 30.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.5, 0.5))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_EQUIVALENT);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SCORE_EQUIVALENCE);
    printf("S6 PASS opt=EQUIVALENT analysis=SCORE_EQUIVALENCE scores=0.5/0.5\n");
    return 0;
}

static int case_s7(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S7\n");
    if (load_latency(&p))
        return 1;
    p.min_improve = 0.74;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 40.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 100.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.875, 0.125))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_SELECT && wl.recommended_policy == 0u);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SELECT_RR);
    printf("S7 PASS opt=SELECT pol=0 analysis=SELECT_RR min_improve=0.74\n");
    return 0;
}

static int case_s8(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S8\n");
    if (load_latency(&p))
        return 1;
    p.min_improve = 0.76;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 40.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 100.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.875, 0.125))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_EQUIVALENT);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SCORE_EQUIVALENCE);
    printf("S8 PASS opt=EQUIVALENT analysis=SCORE_EQUIVALENCE min_improve=0.76\n");
    return 0;
}

static int case_s9(void) {
    haeun_opt_profile p;
    haeun_opt_wl_result wl;
    printf("RUN S9\n");
    if (load_latency(&p))
        return 1;
    p.min_improve = 0.74;
    memset(&wl, 0, sizeof(wl));
    wl.cand_count = 2u;
    plant_latency(&wl.cands[0], 0u, 100.0, 50.0, 1, 1);
    plant_latency(&wl.cands[1], 1u, 40.0, 50.0, 1, 1);
    CHECK(haeun_opt_select(&p, &wl) == HAEUN_OPT_OK);
    if (check_scores(&wl, 0.125, 0.875))
        return 1;
    CHECK(wl.decision == HAEUN_OPT_DEC_SELECT && wl.recommended_policy == 1u);
    CHECK(map_analysis_label(&wl, 0) == R47_LABEL_SELECT_FIFO);
    printf("S9 PASS opt=SELECT pol=1 analysis=SELECT_FIFO min_improve=0.74\n");
    return 0;
}

/** Entry for R4.7 Gate A discrimination unit tests. */
int main(void) {
    int failed = 0;
    failed |= case_s1();
    failed |= case_s2();
    failed |= case_s3();
    failed |= case_s4();
    failed |= case_s5();
    failed |= case_s6();
    failed |= case_s7();
    failed |= case_s8();
    failed |= case_s9();
    if (failed) {
        fprintf(stderr, "r47 discrimination unit tests FAILED\n");
        return 1;
    }
    printf("r47 discrimination unit tests passed (S1-S9)\n");
    return 0;
}
