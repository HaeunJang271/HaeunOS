/**
 * Host-side scheduler evaluation CLI (does not alter guest scheduling).
 * Usage: haeun-eval <build-dir> <workload> [reps] [--json] [--policy rr|fcfs]
 */
#include "haeun/eval_cost.h"
#include "haeun/eval_report.h"
#include "haeun/eval_runner.h"
#include "haeun/eval_workload.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Runs one named workload experiment and prints text or JSON.
 */
int main(int argc, char **argv) {
    haeun_eval_workload wl;
    haeun_eval_experiment exp;
    haeun_eval_report_bundle bundle;
    haeun_eval_cost_weights weights;
    unsigned reps = HAEUN_EVAL_DEFAULT_REPS;
    uint32_t policy = HAEUN_EVAL_POLICY_RR;
    int json = 0;
    const char *dir;
    const char *name;
    int i;
    if (argc < 3) {
        fprintf(stderr,
                "usage: haeun-eval <build-dir> <workload> [reps] [--json] "
                "[--policy rr|fcfs]\n");
        return 2;
    }
    dir = argv[1];
    name = argv[2];
    for (i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0)
            json = 1;
        else if (strcmp(argv[i], "--policy") == 0 && i + 1 < argc) {
            ++i;
            if (strcmp(argv[i], "fcfs") == 0)
                policy = HAEUN_EVAL_POLICY_FCFS;
            else if (strcmp(argv[i], "rr") == 0)
                policy = HAEUN_EVAL_POLICY_RR;
            else {
                fprintf(stderr, "haeun-eval: unknown policy '%s'\n", argv[i]);
                return 2;
            }
        } else
            reps = (unsigned)strtoul(argv[i], NULL, 10);
    }
    if (haeun_eval_workload_by_name(name, &wl) != HAEUN_EVAL_OK)
        return 1;
    if (haeun_eval_experiment_run(&wl, dir, policy, name, reps, &exp) !=
        HAEUN_EVAL_OK)
        return 1;
    haeun_eval_cost_default_weights(&weights);
    if (haeun_eval_report_prepare(&exp, &weights, &bundle) != HAEUN_EVAL_OK)
        return 1;
    if (json)
        haeun_eval_report_fprint_json(stdout, &bundle);
    else
        haeun_eval_report_fprint(stdout, &bundle);
    return exp.fail_count != 0u ? 1 : 0;
}
