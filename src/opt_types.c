#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

static void obj(haeun_opt_objective *o, const char *name, haeun_opt_dir d,
                double w, double reg, unsigned min_s) {
    memset(o, 0, sizeof(*o));
    snprintf(o->metric_name, sizeof(o->metric_name), "%s", name);
    o->direction = d;
    o->weight = w;
    o->regress_limit = reg;
    o->min_samples = min_s;
}

/**
 * Loads named objective profile (experimental defaults).
 */
haeun_opt_result haeun_opt_profile_get(haeun_opt_profile_id id,
                                       haeun_opt_profile *out) {
    if (out == NULL || id >= HAEUN_OPT_PROF_COUNT)
        return HAEUN_OPT_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->id = id;
    out->min_improve = 0.05;
    switch (id) {
    case HAEUN_OPT_PROF_BALANCED:
        snprintf(out->name, sizeof(out->name), "balanced");
        obj(&out->objs[0], "avg_ready_wait", HAEUN_OPT_DIR_MIN, 0.40, 0.10, 2u);
        obj(&out->objs[1], "jain_fairness", HAEUN_OPT_DIR_MAX, 0.30, 0.05, 2u);
        obj(&out->objs[2], "dispatch_count", HAEUN_OPT_DIR_MIN, 0.20, 0.20, 2u);
        obj(&out->objs[3], "completed", HAEUN_OPT_DIR_MAX, 0.10, 0.0, 1u);
        out->obj_count = 4u;
        snprintf(out->rationale, sizeof(out->rationale),
                 "mix latency, fairness, overhead; experimental");
        break;
    case HAEUN_OPT_PROF_FAIRNESS:
        snprintf(out->name, sizeof(out->name), "fairness");
        obj(&out->objs[0], "jain_fairness", HAEUN_OPT_DIR_MAX, 0.70, 0.02, 2u);
        obj(&out->objs[1], "avg_ready_wait", HAEUN_OPT_DIR_MIN, 0.30, 0.15, 2u);
        out->obj_count = 2u;
        snprintf(out->rationale, sizeof(out->rationale),
                 "prioritize Jain fairness on run_ticks");
        break;
    case HAEUN_OPT_PROF_LATENCY:
        snprintf(out->name, sizeof(out->name), "latency");
        obj(&out->objs[0], "avg_ready_wait", HAEUN_OPT_DIR_MIN, 0.75, 0.05, 2u);
        obj(&out->objs[1], "dispatch_count", HAEUN_OPT_DIR_MIN, 0.25, 0.25, 2u);
        out->obj_count = 2u;
        snprintf(out->rationale, sizeof(out->rationale),
                 "minimize ready wait (guest ticks)");
        break;
    case HAEUN_OPT_PROF_THROUGHPUT:
        snprintf(out->name, sizeof(out->name), "throughput");
        obj(&out->objs[0], "completed", HAEUN_OPT_DIR_MAX, 0.60, 0.0, 1u);
        obj(&out->objs[1], "avg_ready_wait", HAEUN_OPT_DIR_MIN, 0.40, 0.20, 2u);
        out->obj_count = 2u;
        snprintf(out->rationale, sizeof(out->rationale),
                 "prefer completions when available");
        break;
    case HAEUN_OPT_PROF_LOW_OVERHEAD:
        snprintf(out->name, sizeof(out->name), "low_overhead");
        obj(&out->objs[0], "dispatch_count", HAEUN_OPT_DIR_MIN, 0.70, 0.10, 2u);
        obj(&out->objs[1], "avg_ready_wait", HAEUN_OPT_DIR_MIN, 0.30, 0.20, 2u);
        out->obj_count = 2u;
        snprintf(out->rationale, sizeof(out->rationale),
                 "minimize dispatch as coarse overhead proxy");
        break;
    default:
        return HAEUN_OPT_BAD_ARGUMENT;
    }
    return HAEUN_OPT_OK;
}

/**
 * Parses profile name; returns -1 if unknown.
 */
int haeun_opt_profile_parse(const char *name, haeun_opt_profile_id *out) {
    haeun_opt_profile_id i;
    haeun_opt_profile p;
    if (name == NULL || out == NULL)
        return -1;
    for (i = 0; i < HAEUN_OPT_PROF_COUNT; ++i) {
        if (haeun_opt_profile_get(i, &p) != HAEUN_OPT_OK)
            continue;
        if (strcmp(name, p.name) == 0) {
            *out = i;
            return 0;
        }
    }
    return -1;
}

/**
 * Default optimizer config (reps=3, balanced).
 */
void haeun_opt_config_default(haeun_opt_config *cfg) {
    if (cfg == NULL)
        return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->profile_id = HAEUN_OPT_PROF_BALANCED;
    cfg->reps = HAEUN_OPT_DEFAULT_REPS;
    cfg->min_success = 2u;
    cfg->min_improve = 0.05;
    cfg->cooldown_cycles = 1u;
    cfg->experimental = 1;
    snprintf(cfg->build_dir, sizeof(cfg->build_dir), "build");
    snprintf(cfg->software_version, sizeof(cfg->software_version), "r36-opt-1");
    cfg->config_fingerprint =
        (uint32_t)cfg->profile_id * 1000u + cfg->reps * 10u + cfg->min_success;
}

/**
 * Initializes optimizer state in SHADOW mode.
 */
void haeun_opt_state_init(haeun_opt_state *st) {
    if (st == NULL)
        return;
    memset(st, 0, sizeof(*st));
    st->schema_version = HAEUN_OPT_SCHEMA;
    snprintf(st->controller_version, sizeof(st->controller_version),
             "r36-opt-1");
    st->mode = HAEUN_OPT_MODE_SHADOW;
    haeun_opt_config_default(&st->cfg);
    (void)haeun_opt_profile_get(st->cfg.profile_id, &st->profile);
}
