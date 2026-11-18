#include "haeun/robust.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static const char *ov_name(haeun_rb_overall o) {
    switch (o) {
    case HAEUN_RB_OVERALL_PASS:
        return "PASS";
    case HAEUN_RB_OVERALL_WARN:
        return "WARN";
    case HAEUN_RB_OVERALL_FAIL:
        return "FAIL";
    default:
        return "UNKNOWN";
    }
}

/**
 * Human-readable robustness report.
 */
void haeun_rb_fprint(FILE *fp, const haeun_rb_state *st) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp, "HaeunOS Robustness Validation (R3.7)\n");
    fprintf(fp, "schema=%u controller=%s preset=%u reps=%u profile=%u hash=0x%08x\n",
            st->schema_version, st->controller_version, (unsigned)st->cfg.preset,
            st->cfg.reps, (unsigned)st->cfg.profile_id, st->cfg.config_hash);
    fprintf(fp, "overall=%s reason=%s\n", ov_name(st->overall),
            st->overall_reason);
    fprintf(fp, "stress transitions=%u failures=%u\n", st->stress_transitions,
            st->stress_failures);
    fprintf(fp, "limitations: %s\n", st->limitations);
    for (i = 0; i < st->wl_count; ++i) {
        const haeun_rb_wl_report *w = &st->workloads[i];
        fprintf(fp,
                "  wl=%s seed=0x%08x replay=%d rel=%d sens=%d verify=%d "
                "orig_pol=%u dec=%d\n",
                w->workload_id, w->seed, (int)w->replay, (int)w->reliability,
                (int)w->sensitivity, (int)w->verify, w->original_policy,
                (int)w->original_decision);
    }
}

/**
 * JSON report with schema_version.
 */
void haeun_rb_fprint_json(FILE *fp, const haeun_rb_state *st) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":%u,\"build_identifier\":\"%s\","
            "\"source_revision\":\"%s\",\"experiment_configuration\":{"
            "\"preset\":%u,\"reps\":%u,\"profile_id\":%u,\"config_hash\":%u},"
            "\"overall\":\"%s\",\"overall_reason\":\"%s\","
            "\"stress\":{\"transitions\":%u,\"failures\":%u},"
            "\"workloads\":[",
            st->schema_version, st->cfg.build_id, st->cfg.source_revision,
            (unsigned)st->cfg.preset, st->cfg.reps,
            (unsigned)st->cfg.profile_id, st->cfg.config_hash,
            ov_name(st->overall), st->overall_reason, st->stress_transitions,
            st->stress_failures);
    for (i = 0; i < st->wl_count; ++i) {
        const haeun_rb_wl_report *w = &st->workloads[i];
        fprintf(fp,
                "%s{\"workload_id\":\"%s\",\"seed\":%u,\"replay\":%d,"
                "\"reliability\":%d,\"sensitivity\":%d,\"verify\":%d,"
                "\"original_policy\":%u,\"verified_policy\":%u}",
                i ? "," : "", w->workload_id, w->seed, (int)w->replay,
                (int)w->reliability, (int)w->sensitivity, (int)w->verify,
                w->original_policy, w->verified_policy);
    }
    fprintf(fp, "],\"limitations\":\"%s\"}\n", st->limitations);
}

/**
 * Saves compact state JSON.
 */
haeun_rb_result haeun_rb_state_save(FILE *fp, const haeun_rb_state *st) {
    if (fp == NULL || st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    if (st->schema_version != HAEUN_RB_SCHEMA)
        return HAEUN_RB_BAD_STATE;
    haeun_rb_fprint_json(fp, st);
    return HAEUN_RB_OK;
}

/**
 * Loads schema_version only (full replay from experiments dir is separate).
 */
haeun_rb_result haeun_rb_state_load(FILE *fp, haeun_rb_state *st) {
    char buf[4096];
    size_t n;
    const char *p;
    if (fp == NULL || st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    n = fread(buf, 1u, sizeof(buf) - 1u, fp);
    buf[n] = '\0';
    haeun_rb_state_init(st);
    p = strstr(buf, "\"schema_version\"");
    if (p == NULL)
        return HAEUN_RB_BAD_STATE;
    p = strchr(p, ':');
    if (p == NULL || (unsigned)strtoul(p + 1, NULL, 10) != HAEUN_RB_SCHEMA)
        return HAEUN_RB_BAD_STATE;
    return HAEUN_RB_OK;
}

/**
 * Write reports under out_dir.
 */
haeun_rb_result haeun_rb_export(const haeun_rb_state *st, const char *out_dir) {
    char path[HAEUN_EVAL_PATH_MAX];
    FILE *fp;
    if (st == NULL || out_dir == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
#ifdef _WIN32
    (void)_mkdir(out_dir);
#else
    (void)mkdir(out_dir, 0755);
#endif
    snprintf(path, sizeof(path), "%s/robustness-report.json", out_dir);
    fp = fopen(path, "wb");
    if (fp == NULL)
        return HAEUN_RB_IO;
    haeun_rb_fprint_json(fp, st);
    (void)fclose(fp);
    snprintf(path, sizeof(path), "%s/robustness-report.txt", out_dir);
    fp = fopen(path, "wb");
    if (fp == NULL)
        return HAEUN_RB_IO;
    haeun_rb_fprint(fp, st);
    (void)fclose(fp);
    return HAEUN_RB_OK;
}
