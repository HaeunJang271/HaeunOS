/**
 * R2.2 unit tests: stats, metrics, fairness, cost, compare (no guest run).
 */
#include "haeun/eval_compare.h"
#include "haeun/eval_cost.h"
#include "haeun/eval_fairness.h"
#include "haeun/eval_metrics.h"
#include "haeun/eval_report.h"
#include "haeun/eval_stats.h"
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

static int near(double a, double b, double eps) {
    double d = a - b;
    if (d < 0.0)
        d = -d;
    return d <= eps;
}

static int test_stats(void) {
    double v[] = {1.0, 2.0, 3.0, 4.0};
    double one[] = {7.0};
    haeun_eval_stat_summary s;
    printf("RUN stats\n");
    CHECK(haeun_eval_stats_compute(NULL, 0u, &s) == HAEUN_EVAL_OK);
    CHECK(!s.mean.available);
    CHECK(haeun_eval_stats_compute(v, 4u, &s) == HAEUN_EVAL_OK);
    CHECK(s.sample_count == 4u && near(s.mean.value, 2.5, 1e-9));
    CHECK(near(s.median.value, 2.5, 1e-9));
    CHECK(near(s.min.value, 1.0, 1e-9) && near(s.max.value, 4.0, 1e-9));
    CHECK(s.stddev.available);
    CHECK(haeun_eval_stats_compute(one, 1u, &s) == HAEUN_EVAL_OK);
    CHECK(!s.stddev.available);
    printf("stats ok\n");
    return 0;
}

static void fill_slot(haeun_telem_slot *s, uint32_t slot, uint32_t state,
                      uint32_t pid, uint32_t run, uint32_t ready, uint32_t disp,
                      uint32_t cv) {
    memset(s, 0, sizeof(*s));
    s->slot = slot;
    s->state = state;
    s->pid = pid;
    s->run_ticks = run;
    s->ready_wait_ticks = ready;
    s->dispatch_count = disp;
    s->complete_valid = cv;
    s->as_slot_valid = slot < 3u ? 1 : 0;
}

static int test_metrics(void) {
    haeun_telem_report rep;
    haeun_eval_run_metrics m;
    haeun_eval_run_metrics z;
    printf("RUN metrics\n");
    memset(&rep, 0, sizeof(rep));
    fill_slot(&rep.slots[0], 0, 5, 1, 100, 20, 5, 1);
    fill_slot(&rep.slots[1], 1, 0, 0, 99, 0, 0, 0);
    fill_slot(&rep.slots[2], 2, 5, 2, 50, 10, 3, 1);
    fill_slot(&rep.slots[3], 3, 1, 9, 1, 1, 1, 0);
    rep.slots[3].as_slot_valid = 0;
    rep.context_switches = 2;
    rep.timer_irqs = 7;
    CHECK(haeun_eval_metrics_from_telem(&rep, 24, 1000, &m) == HAEUN_EVAL_OK);
    CHECK(m.participants == 2u);
    CHECK(m.total_run_ticks == 150u);
    CHECK(m.total_ready_wait == 30u);
    CHECK(m.avg_ready_wait.available);
    CHECK(m.ctx_dispatch_ratio.available);
    CHECK(near(m.ctx_dispatch_ratio.value, 2.0 / 8.0, 1e-9));
    CHECK(!m.slot_included[1] && !m.slot_included[3]);
    memset(&rep, 0, sizeof(rep));
    fill_slot(&rep.slots[0], 0, 5, 1, 1, 0, 0, 1);
    CHECK(haeun_eval_metrics_from_telem(&rep, 0, 1, &z) == HAEUN_EVAL_OK);
    CHECK(!z.ctx_dispatch_ratio.available);
    printf("metrics ok\n");
    return 0;
}

static int test_fairness_cost(void) {
    haeun_eval_run_metrics m;
    haeun_eval_fairness f;
    haeun_eval_cost_weights w;
    haeun_eval_cost_result c;
    printf("RUN fairness_cost\n");
    memset(&m, 0, sizeof(m));
    m.ok = 1;
    m.participants = 2;
    m.slot_included[0] = 1;
    m.slot_included[1] = 1;
    m.slot_run[0] = 10;
    m.slot_run[1] = 10;
    m.total_ready_wait = 0;
    m.total_dispatch = 10;
    m.context_switches = 0;
    m.ctx_dispatch_ratio.available = 1;
    m.ctx_dispatch_ratio.value = 0.0;
    CHECK(haeun_eval_fairness_from_metrics(&m, &f) == HAEUN_EVAL_OK);
    CHECK(f.jain_run_ticks.available && near(f.jain_run_ticks.value, 1.0, 1e-9));
    m.slot_run[1] = 0;
    CHECK(haeun_eval_fairness_from_metrics(&m, &f) == HAEUN_EVAL_OK);
    CHECK(f.jain_run_ticks.value < 1.0);
    haeun_eval_cost_default_weights(&w);
    CHECK(haeun_eval_cost_compute(&m, &w, &c) == HAEUN_EVAL_OK);
    CHECK(c.experimental && c.score.available);
    w.w_ready_wait = -1.0;
    CHECK(haeun_eval_cost_compute(&m, &w, &c) == HAEUN_EVAL_BAD_ARGUMENT);
    printf("fairness_cost ok\n");
    return 0;
}

static int test_workload_compare(void) {
    haeun_eval_workload wl;
    haeun_eval_experiment ea, eb;
    haeun_eval_report_bundle ba, bb;
    char path[256];
    printf("RUN workload_compare\n");
    CHECK(haeun_eval_workload_by_name("nope", &wl) == HAEUN_EVAL_NO_WORKLOAD);
    CHECK(haeun_eval_workload_get(HAEUN_WL_CPU_BOUND, &wl) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_workload_image_path(&wl, "build", path, sizeof(path)) ==
          HAEUN_EVAL_OK);
    CHECK(strstr(path, "r12_run.hxf") != NULL);
    memset(&ea, 0, sizeof(ea));
    memset(&eb, 0, sizeof(eb));
    ea.workload = wl;
    eb.workload = wl;
    ea.requested_reps = 1;
    eb.requested_reps = 1;
    ea.success_count = 1;
    eb.success_count = 1;
    ea.runs[0].metrics.ok = 1;
    ea.runs[0].metrics.total_run_ticks = 10;
    eb.runs[0].metrics.ok = 1;
    eb.runs[0].metrics.total_run_ticks = 20;
    CHECK(haeun_eval_report_prepare(&ea, NULL, &ba) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_report_prepare(&eb, NULL, &bb) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_compare(&ba, &bb, stdout) == HAEUN_EVAL_OK);
    eb.workload.kind = HAEUN_WL_MIXED;
    snprintf(eb.workload.name, sizeof(eb.workload.name), "mixed");
    CHECK(haeun_eval_report_prepare(&eb, NULL, &bb) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_compare(&ba, &bb, stdout) == HAEUN_EVAL_INCOMPARABLE);
    printf("workload_compare ok\n");
    return 0;
}

/**
 * Entry for R2.2 host-side unit tests.
 */
int main(void) {
    if (test_stats() || test_metrics() || test_fairness_cost() ||
        test_workload_compare())
        return 1;
    printf("r22 eval unit tests passed\n");
    return 0;
}
