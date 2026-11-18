#include "haeun/optimize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_u32(const char *s, const char *key, uint32_t *out) {
    const char *p = strstr(s, key);
    unsigned long v;
    char *end;
    if (p == NULL)
        return 0;
    p += strlen(key);
    while (*p && (*p == '"' || *p == ':' || *p == ' '))
        p++;
    v = strtoul(p, &end, 10);
    if (end == p)
        return 0;
    *out = (uint32_t)v;
    return 1;
}

/**
 * Validates loaded state invariants.
 */
haeun_opt_result haeun_opt_state_validate(const haeun_opt_state *st) {
    if (st == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    if (st->schema_version != HAEUN_OPT_SCHEMA)
        return HAEUN_OPT_BAD_STATE;
    if (st->wl_count > HAEUN_OPT_WL_MAX || st->hist_count > HAEUN_OPT_HIST_MAX)
        return HAEUN_OPT_BAD_STATE;
    if (st->cfg.profile_id >= HAEUN_OPT_PROF_COUNT)
        return HAEUN_OPT_BAD_STATE;
    return HAEUN_OPT_OK;
}

/**
 * Saves state as deterministic JSON.
 */
haeun_opt_result haeun_opt_state_save(FILE *fp, const haeun_opt_state *st) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    if (haeun_opt_state_validate(st) != HAEUN_OPT_OK)
        return HAEUN_OPT_BAD_STATE;
    fprintf(fp,
            "{\"schema_version\":%u,\"controller_version\":\"%s\","
            "\"mode\":%d,\"cycle_id\":%u,\"profile\":\"%s\",\"reps\":%u,"
            "\"config_fingerprint\":%u,\"wl_count\":%u,\"workloads\":[",
            st->schema_version, st->controller_version, (int)st->mode,
            st->cycle_id, st->profile.name, st->cfg.reps,
            st->cfg.config_fingerprint, st->wl_count);
    for (i = 0; i < st->wl_count; ++i) {
        const haeun_opt_wl_result *w = &st->workloads[i];
        fprintf(fp,
                "%s{\"workload_id\":\"%s\",\"decision\":%d,"
                "\"recommended_policy\":%u,\"reason\":\"%s\"}",
                i ? "," : "", w->workload_id, (int)w->decision,
                w->recommended_policy, w->reason);
    }
    fprintf(fp,
            "],\"last_rec\":{\"valid\":%d,\"decision\":%d,\"policy\":%u,"
            "\"generation\":%u,\"experiment_id\":\"%s\"}}\n",
            st->last_rec.valid, (int)st->last_rec.decision,
            st->last_rec.recommended_policy, st->last_rec.generation,
            st->last_rec.experiment_id);
    return HAEUN_OPT_OK;
}

/**
 * Loads state; incompatible schema → BAD_STATE.
 */
haeun_opt_result haeun_opt_state_load(FILE *fp, haeun_opt_state *st) {
    char buf[8192];
    size_t n;
    uint32_t schema = 0u;
    if (fp == NULL || st == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    n = fread(buf, 1u, sizeof(buf) - 1u, fp);
    buf[n] = '\0';
    haeun_opt_state_init(st);
    if (!parse_u32(buf, "\"schema_version\"", &schema) ||
        schema != HAEUN_OPT_SCHEMA)
        return HAEUN_OPT_BAD_STATE;
    st->schema_version = schema;
    (void)parse_u32(buf, "\"cycle_id\"", &st->cycle_id);
    (void)parse_u32(buf, "\"config_fingerprint\"", &st->cfg.config_fingerprint);
    (void)parse_u32(buf, "\"generation\"", &st->last_rec.generation);
    if (strstr(buf, "\"valid\":1") != NULL)
        st->last_rec.valid = 1;
    return haeun_opt_state_validate(st);
}
