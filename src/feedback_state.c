#include "haeun/feedback.h"

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

static int parse_u(const char *s, const char *key, unsigned *out) {
    uint32_t v = 0u;
    if (!parse_u32(s, key, &v))
        return 0;
    *out = (unsigned)v;
    return 1;
}

/**
 * Validates loaded state invariants.
 */
haeun_fb_result haeun_fb_state_validate(const haeun_fb_state *st) {
    if (st == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (st->schema_version != HAEUN_FB_SCHEMA)
        return HAEUN_FB_BAD_STATE;
    if (st->hist_count > HAEUN_FB_HIST_MAX)
        return HAEUN_FB_BAD_STATE;
    if (st->cfg.obj_count > HAEUN_FB_OBJ_MAX)
        return HAEUN_FB_BAD_STATE;
    return HAEUN_FB_OK;
}

/**
 * Saves state as deterministic JSON.
 */
haeun_fb_result haeun_fb_state_save(FILE *fp, const haeun_fb_state *st) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (haeun_fb_state_validate(st) != HAEUN_FB_OK)
        return HAEUN_FB_BAD_STATE;
    fprintf(fp,
            "{\"schema_version\":%u,\"controller_version\":\"%s\","
            "\"cycle_id\":%u,\"mode\":%d,\"current_policy\":%u,"
            "\"previous_policy\":%u,\"previous_decision\":%d,"
            "\"previous_outcome\":%d,\"previous_outcome_valid\":%d,"
            "\"workload_fingerprint\":%u,\"measurement_generation\":%u,"
            "\"cooldown_remaining\":%u,\"consecutive_improvements\":%u,"
            "\"consecutive_regressions\":%u,\"hist_count\":%u,\"history\":[",
            st->schema_version, st->controller_version, st->cycle_id,
            (int)st->mode, st->current_policy, st->previous_policy,
            (int)st->previous_decision, (int)st->previous_outcome,
            st->previous_outcome_valid, st->workload_fingerprint,
            st->measurement_generation, st->cooldown_remaining,
            st->consecutive_improvements, st->consecutive_regressions,
            st->hist_count);
    for (i = 0; i < st->hist_count; ++i) {
        const haeun_fb_hist_entry *e = &st->history[i];
        fprintf(fp,
                "%s{\"cycle_id\":%u,\"from_policy\":%u,\"to_policy\":%u,"
                "\"decision\":%d,\"outcome\":%d,\"txn_committed\":%d}",
                i ? "," : "", e->cycle_id, e->from_policy, e->to_policy,
                (int)e->decision, (int)e->outcome, e->txn_committed);
    }
    fprintf(fp,
            "],\"cfg\":{\"obj_count\":%u,\"min_valid_objectives\":%u,"
            "\"consecutive_improve_needed\":%u,\"consecutive_regress_needed\":%u,"
            "\"cooldown_cycles\":%u,\"experimental\":%d}}\n",
            st->cfg.obj_count, st->cfg.min_valid_objectives,
            st->cfg.consecutive_improve_needed,
            st->cfg.consecutive_regress_needed, st->cfg.cooldown_cycles,
            st->cfg.experimental);
    return HAEUN_FB_OK;
}

/**
 * Loads state; incompatible schema → BAD_STATE.
 */
haeun_fb_result haeun_fb_state_load(FILE *fp, haeun_fb_state *st) {
    char *buf;
    long n;
    uint32_t schema = 0u;
    if (fp == NULL || st == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    if (fseek(fp, 0, SEEK_END) != 0)
        return HAEUN_FB_IO;
    n = ftell(fp);
    if (n < 2 || n > 65536)
        return HAEUN_FB_IO;
    if (fseek(fp, 0, SEEK_SET) != 0)
        return HAEUN_FB_IO;
    buf = (char *)malloc((size_t)n + 1u);
    if (buf == NULL)
        return HAEUN_FB_IO;
    if (fread(buf, 1u, (size_t)n, fp) != (size_t)n) {
        free(buf);
        return HAEUN_FB_IO;
    }
    buf[n] = '\0';
    haeun_fb_state_init(st);
    if (!parse_u32(buf, "\"schema_version\"", &schema) ||
        schema != HAEUN_FB_SCHEMA) {
        free(buf);
        return HAEUN_FB_BAD_STATE;
    }
    st->schema_version = schema;
    (void)parse_u32(buf, "\"cycle_id\"", &st->cycle_id);
    {
        uint32_t mode = 0u;
        if (parse_u32(buf, "\"mode\"", &mode))
            st->mode = (haeun_fb_mode)mode;
    }
    (void)parse_u32(buf, "\"current_policy\"", &st->current_policy);
    (void)parse_u32(buf, "\"previous_policy\"", &st->previous_policy);
    {
        uint32_t v = 0u;
        if (parse_u32(buf, "\"previous_decision\"", &v))
            st->previous_decision = (haeun_plan_decision)v;
        if (parse_u32(buf, "\"previous_outcome\"", &v))
            st->previous_outcome = (haeun_fb_outcome)v;
        if (parse_u32(buf, "\"previous_outcome_valid\"", &v))
            st->previous_outcome_valid = (int)v;
    }
    (void)parse_u32(buf, "\"workload_fingerprint\"", &st->workload_fingerprint);
    (void)parse_u32(buf, "\"measurement_generation\"",
                    &st->measurement_generation);
    (void)parse_u32(buf, "\"cooldown_remaining\"", &st->cooldown_remaining);
    (void)parse_u(buf, "\"consecutive_improvements\"",
                  &st->consecutive_improvements);
    (void)parse_u(buf, "\"consecutive_regressions\"",
                  &st->consecutive_regressions);
    (void)parse_u(buf, "\"hist_count\"", &st->hist_count);
    if (st->hist_count > HAEUN_FB_HIST_MAX)
        st->hist_count = 0u;
    free(buf);
    return haeun_fb_state_validate(st);
}
