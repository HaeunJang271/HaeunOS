#include "haeun/robust.h"

#include "haeun/eval_workload.h"

#include <stdio.h>
#include <string.h>

static uint32_t fnv1a(const void *data, size_t n, uint32_t h) {
    const uint8_t *p = (const uint8_t *)data;
    size_t i;
    for (i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

/**
 * Initializes robustness state / default smoke preset.
 */
void haeun_rb_state_init(haeun_rb_state *st) {
    if (st == NULL)
        return;
    memset(st, 0, sizeof(*st));
    st->schema_version = HAEUN_RB_SCHEMA;
    snprintf(st->controller_version, sizeof(st->controller_version),
             "r37-rb-1");
    (void)haeun_rb_preset_apply(&st->cfg, HAEUN_RB_PRESET_SMOKE);
    snprintf(st->cfg.build_dir, sizeof(st->cfg.build_dir), "build");
    snprintf(st->cfg.build_id, sizeof(st->cfg.build_id), "local");
    snprintf(st->cfg.source_revision, sizeof(st->cfg.source_revision),
             "unknown");
    st->cfg.profile_id = HAEUN_OPT_PROF_BALANCED;
    st->cfg.config_hash = haeun_rb_config_hash(&st->cfg);
    st->overall = HAEUN_RB_OVERALL_UNKNOWN;
    snprintf(st->limitations, sizeof(st->limitations),
             "descriptive stats; catalog workloads have no RNG");
}

/**
 * Applies experiment preset.
 */
haeun_rb_result haeun_rb_preset_apply(haeun_rb_config *cfg,
                                      haeun_rb_preset preset) {
    if (cfg == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    cfg->preset = preset;
    cfg->min_success = 2u;
    switch (preset) {
    case HAEUN_RB_PRESET_SMOKE:
        cfg->reps = 3u;
        break;
    case HAEUN_RB_PRESET_STANDARD:
        cfg->reps = 10u;
        break;
    case HAEUN_RB_PRESET_EXTENDED:
        cfg->reps = 30u;
        break;
    case HAEUN_RB_PRESET_RESEARCH:
        cfg->reps = 100u;
        break;
    default:
        return HAEUN_RB_BAD_ARGUMENT;
    }
    if (cfg->reps < cfg->min_success)
        cfg->min_success = cfg->reps ? cfg->reps : 1u;
    cfg->config_hash = haeun_rb_config_hash(cfg);
    return HAEUN_RB_OK;
}

/**
 * Parses preset name.
 */
int haeun_rb_preset_parse(const char *name, haeun_rb_preset *out) {
    if (name == NULL || out == NULL)
        return -1;
    if (strcmp(name, "smoke") == 0) {
        *out = HAEUN_RB_PRESET_SMOKE;
        return 0;
    }
    if (strcmp(name, "standard") == 0) {
        *out = HAEUN_RB_PRESET_STANDARD;
        return 0;
    }
    if (strcmp(name, "extended") == 0) {
        *out = HAEUN_RB_PRESET_EXTENDED;
        return 0;
    }
    if (strcmp(name, "research") == 0) {
        *out = HAEUN_RB_PRESET_RESEARCH;
        return 0;
    }
    return -1;
}

/**
 * Deterministic seed for catalog workload+policy+step_cap.
 */
uint32_t haeun_rb_workload_seed(haeun_eval_wl_kind kind, uint32_t policy_id) {
    haeun_eval_workload wl;
    uint32_t h = 2166136261u;
    char buf[32];
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return 0u;
    h = fnv1a(wl.name, strlen(wl.name), h);
    h = fnv1a(wl.image_name, strlen(wl.image_name), h);
    snprintf(buf, sizeof(buf), "%u:%llu:%u", policy_id,
             (unsigned long long)wl.step_cap, wl.expected_min_pids);
    h = fnv1a(buf, strlen(buf), h);
    return h;
}

/**
 * Configuration hash.
 */
uint32_t haeun_rb_config_hash(const haeun_rb_config *cfg) {
    uint32_t h = 2166136261u;
    char buf[96];
    if (cfg == NULL)
        return 0u;
    h = fnv1a(cfg->build_dir, strlen(cfg->build_dir), h);
    snprintf(buf, sizeof(buf), "%u:%u:%u:%u", (unsigned)cfg->preset, cfg->reps,
             cfg->min_success, (unsigned)cfg->profile_id);
    return fnv1a(buf, strlen(buf), h);
}
