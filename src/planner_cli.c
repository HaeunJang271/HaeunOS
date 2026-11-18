#include "haeun/planner.h"

/**
 * CLI entry: plan (shadow default). execute_opt-in commits via R3.2 only.
 */
int haeun_plan_cli_run(haeun_vm *vm, int json, int execute) {
    haeun_plan_session sess;
    haeun_plan plan;
    haeun_plan_result pr;
    int shadow;
    if (vm == NULL)
        return 2;
    haeun_plan_session_init(&sess);
    shadow = execute ? 0 : 1;
    pr = haeun_plan_run(&sess, vm, shadow, execute ? 1 : 0, &plan);
    if (json)
        haeun_plan_fprint_json(stdout, &plan);
    else
        haeun_plan_fprint(stdout, &plan);
    if (pr != HAEUN_PLAN_OK && pr != HAEUN_PLAN_REJECTED)
        return 1;
    /* Shadow success even if REJECTED_BY_SAFETY — that is a valid plan outcome. */
    return 0;
}
