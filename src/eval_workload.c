#include "haeun/eval_workload.h"

#include <stdio.h>
#include <string.h>

static const haeun_eval_workload CATALOG[HAEUN_WL_COUNT] = {
    {HAEUN_WL_CPU_BOUND, "cpu_bound", "cpu", "r12_run", 21u, 1u, 200000u, 0u, 0},
    {HAEUN_WL_SHORT_JOB, "short_job", "short", "r12_yield", 22u, 1u, 200000u, 0u,
     0},
    /* Final guest r1 is child EXIT 25 (parent 24 completes earlier). */
    {HAEUN_WL_MIXED, "mixed", "mixed", "r12_pair", 25u, 2u, 400000u, 0u, 0},
    {HAEUN_WL_BURSTY, "bursty", "bursty", "r22_burst", 26u, 1u, 300000u, 0u, 0},
    {HAEUN_WL_BLOCK_SLEEP, "block_sleep", "blocking", "r12_sleep", 23u, 1u,
     300000u, 0u, 0},
    {HAEUN_WL_BLOCK_WAIT, "block_wait", "blocking", "r12_wait", 5u, 2u, 200000u,
     0u, 0},
};

/**
 * Fills the static catalog entry for kind.
 */
haeun_eval_result haeun_eval_workload_get(haeun_eval_wl_kind kind,
                                          haeun_eval_workload *out) {
    if (out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    if (kind < 0 || kind >= HAEUN_WL_COUNT)
        return HAEUN_EVAL_NO_WORKLOAD;
    *out = CATALOG[kind];
    return HAEUN_EVAL_OK;
}

/**
 * Looks up a workload by name.
 */
haeun_eval_result haeun_eval_workload_by_name(const char *name,
                                              haeun_eval_workload *out) {
    int k;
    if (name == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    for (k = 0; k < (int)HAEUN_WL_COUNT; ++k) {
        if (strcmp(CATALOG[k].name, name) == 0) {
            *out = CATALOG[k];
            return HAEUN_EVAL_OK;
        }
    }
    fprintf(stderr, "haeun-eval: unknown workload '%s'\n", name);
    return HAEUN_EVAL_NO_WORKLOAD;
}

/**
 * Builds absolute HXF path under build_dir.
 */
haeun_eval_result haeun_eval_workload_image_path(const haeun_eval_workload *wl,
                                                 const char *build_dir,
                                                 char *path, size_t path_cap) {
    int n;
    if (wl == NULL || build_dir == NULL || path == NULL || path_cap == 0u)
        return HAEUN_EVAL_BAD_ARGUMENT;
    n = snprintf(path, path_cap, "%s/%s.hxf", build_dir, wl->image_name);
    if (n < 0 || (size_t)n >= path_cap)
        return HAEUN_EVAL_IO;
    return HAEUN_EVAL_OK;
}

/**
 * Returns 1 when kind is in the supported catalog.
 */
int haeun_eval_workload_supported(haeun_eval_wl_kind kind) {
    return kind >= 0 && kind < HAEUN_WL_COUNT;
}
