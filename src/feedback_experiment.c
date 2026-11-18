#include "haeun/feedback_experiment.h"

#include "haeun/eval_fairness.h"
#include "haeun/eval_runner.h"
#include "haeun/eval_stats.h"
#include "haeun/eval_workload.h"
#include "haeun/feedback.h"
#include "haeun/hxf.h"
#include "haeun/planner.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAM_SIZE = 16u * 1024u * 1024u };

static void fill_stats(haeun_fb_exp_result *out, const haeun_eval_experiment *ex) {
    double ready[HAEUN_EVAL_MAX_RUNS];
    double jain[HAEUN_EVAL_MAX_RUNS];
    double disp[HAEUN_EVAL_MAX_RUNS];
    unsigned n = 0u, i;
    haeun_eval_stat_summary sr, sj, sd;
    for (i = 0; i < ex->success_count && i < HAEUN_EVAL_MAX_RUNS; ++i) {
        const haeun_eval_run_metrics *m = &ex->runs[i].metrics;
        haeun_eval_fairness f;
        if (!m->ok)
            continue;
        if (m->avg_ready_wait.available)
            ready[n] = m->avg_ready_wait.value;
        else
            continue;
        if (haeun_eval_fairness_from_metrics(m, &f) != HAEUN_EVAL_OK ||
            !f.jain_run_ticks.available)
            continue;
        jain[n] = f.jain_run_ticks.value;
        disp[n] = (double)m->total_dispatch;
        n++;
    }
    memset(&out->mean_ready, 0, sizeof(out->mean_ready));
    memset(&out->mean_jain, 0, sizeof(out->mean_jain));
    memset(&out->mean_dispatch, 0, sizeof(out->mean_dispatch));
    if (n == 0u)
        return;
    if (haeun_eval_stats_compute(ready, n, &sr) == HAEUN_EVAL_OK)
        out->mean_ready = sr.mean;
    if (haeun_eval_stats_compute(jain, n, &sj) == HAEUN_EVAL_OK)
        out->mean_jain = sj.mean;
    if (haeun_eval_stats_compute(disp, n, &sd) == HAEUN_EVAL_OK)
        out->mean_dispatch = sd.mean;
}

/**
 * Runs fixed RR or FIFO experiment via R2.2 runner.
 */
haeun_fb_result haeun_fb_exp_fixed(const char *build_dir,
                                   haeun_eval_wl_kind kind, uint32_t policy_id,
                                   unsigned reps, haeun_fb_exp_result *out) {
    haeun_eval_workload wl;
    haeun_eval_experiment ex;
    char id[64];
    if (build_dir == NULL || out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return HAEUN_FB_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->schema_version = HAEUN_FB_EXP_SCHEMA;
    out->mode = (policy_id == HAEUN_EVAL_POLICY_FCFS) ? HAEUN_FB_EXP_FIXED_FIFO
                                                      : HAEUN_FB_EXP_FIXED_RR;
    snprintf(out->workload_id, sizeof(out->workload_id), "%s", wl.name);
    out->reps = reps;
    snprintf(id, sizeof(id), "fb-%u-%u", (unsigned)kind, policy_id);
    if (haeun_eval_experiment_run(&wl, build_dir, policy_id, id, reps, &ex) !=
        HAEUN_EVAL_OK)
        return HAEUN_FB_IO;
    out->success = ex.success_count;
    out->fail = ex.fail_count;
    out->transitions = 0u;
    fill_stats(out, &ex);
    snprintf(out->limitations, sizeof(out->limitations),
             "fixed policy; reps=%u not statistical proof", reps);
    return HAEUN_FB_OK;
}

static int load_run(const char *path, haeun_vm *vm, uint8_t **ram_out) {
    FILE *f;
    uint8_t *image, *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return -1;
    }
    image = (uint8_t *)malloc((size_t)n);
    if (image == NULL ||
        (got = fread(image, 1u, (size_t)n, f)) != (size_t)n) {
        free(image);
        (void)fclose(f);
        return -1;
    }
    (void)fclose(f);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    if (ram == NULL ||
        haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) !=
            HAEUN_HXF_OK) {
        free(image);
        free(ram);
        return -1;
    }
    free(image);
    if (haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) != HAEUN_VM_OK) {
        free(ram);
        return -1;
    }
    *ram_out = ram;
    return 0;
}

/**
 * Runs Adaptive cycles over a workload image (SHADOW unless execute=1).
 */
haeun_fb_result haeun_fb_exp_adaptive(const char *build_dir,
                                      haeun_eval_wl_kind kind, unsigned reps,
                                      int execute, haeun_fb_exp_result *out) {
    haeun_eval_workload wl;
    char path[HAEUN_EVAL_PATH_MAX];
    unsigned r;
    unsigned transitions = 0u;
    if (build_dir == NULL || out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return HAEUN_FB_BAD_ARGUMENT;
    if (haeun_eval_workload_image_path(&wl, build_dir, path, sizeof(path)) !=
        HAEUN_EVAL_OK)
        return HAEUN_FB_IO;
    memset(out, 0, sizeof(*out));
    out->schema_version = HAEUN_FB_EXP_SCHEMA;
    out->mode = HAEUN_FB_EXP_ADAPTIVE;
    snprintf(out->workload_id, sizeof(out->workload_id), "%s", wl.name);
    out->reps = reps == 0u ? 1u : reps;
    for (r = 0; r < out->reps; ++r) {
        haeun_vm vm;
        uint8_t *ram = NULL;
        haeun_fb_state st;
        haeun_plan_session sess;
        haeun_plan plan;
        haeun_fb_assessment a;
        uint64_t i;
        if (load_run(path, &vm, &ram) != 0) {
            out->fail++;
            continue;
        }
        haeun_fb_state_init(&st);
        haeun_plan_session_init(&sess);
        st.mode = execute ? HAEUN_FB_MODE_ADAPTIVE : HAEUN_FB_MODE_SHADOW;
        st.workload_fingerprint = (uint32_t)kind + 1u;
        for (i = 0; i < wl.step_cap; ++i) {
            haeun_vm_result s = haeun_vm_step(&vm);
            if (s != HAEUN_VM_OK) {
                if (s == HAEUN_VM_HALTED)
                    break;
                out->fail++;
                free(ram);
                goto next;
            }
            if (i > 2000u && (i % 5000u) == 0u) {
                if (haeun_fb_cycle(&st, &sess, &vm, execute, &plan, &a) ==
                    HAEUN_FB_OK) {
                    if (plan.txn_committed)
                        transitions++;
                }
            }
        }
        out->success++;
        free(ram);
    next:;
    }
    out->transitions = transitions;
    snprintf(out->limitations, sizeof(out->limitations),
             "adaptive %s; not proven superior to fixed; reps=%u",
             execute ? "execute" : "shadow", out->reps);
    return HAEUN_FB_OK;
}

/**
 * Prints one experiment result (text).
 */
void haeun_fb_exp_fprint(FILE *fp, const haeun_fb_exp_result *r) {
    const char *modes[] = {"FIXED_RR", "FIXED_FIFO", "ADAPTIVE"};
    if (fp == NULL || r == NULL)
        return;
    fprintf(fp, "fb-exp mode=%s wl=%s reps=%u ok=%u fail=%u transitions=%u\n",
            modes[r->mode < 3 ? r->mode : 0], r->workload_id, r->reps,
            r->success, r->fail, r->transitions);
    if (r->mean_ready.available)
        fprintf(fp, "  mean_avg_ready_wait=%.4f\n", r->mean_ready.value);
    if (r->mean_jain.available)
        fprintf(fp, "  mean_jain=%.4f\n", r->mean_jain.value);
    if (r->mean_dispatch.available)
        fprintf(fp, "  mean_dispatch=%.4f\n", r->mean_dispatch.value);
    fprintf(fp, "  limitations: %s\n", r->limitations);
}

/**
 * Prints one experiment result (JSON).
 */
void haeun_fb_exp_fprint_json(FILE *fp, const haeun_fb_exp_result *r) {
    const char *modes[] = {"FIXED_RR", "FIXED_FIFO", "ADAPTIVE"};
    if (fp == NULL || r == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":%u,\"mode\":\"%s\",\"workload_id\":\"%s\","
            "\"reps\":%u,\"success\":%u,\"fail\":%u,\"transitions\":%u,"
            "\"mean_avg_ready_wait\":{\"available\":%s,\"value\":%.6f},"
            "\"mean_jain\":{\"available\":%s,\"value\":%.6f},"
            "\"limitations\":\"%s\"}\n",
            r->schema_version, modes[r->mode < 3 ? r->mode : 0], r->workload_id,
            r->reps, r->success, r->fail, r->transitions,
            r->mean_ready.available ? "true" : "false", r->mean_ready.value,
            r->mean_jain.available ? "true" : "false", r->mean_jain.value,
            r->limitations);
}
