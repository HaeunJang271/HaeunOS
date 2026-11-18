#include "haeun/feedback_types.h"

#include <stdio.h>
#include <string.h>

/**
 * Fills conservative experimental multi-objective defaults.
 */
void haeun_fb_config_default(haeun_fb_config *cfg) {
    if (cfg == NULL)
        return;
    memset(cfg, 0, sizeof(*cfg));
    /* Prefer lower average ready wait (guest ticks; not wall-clock). */
    snprintf(cfg->objs[0].metric_name, sizeof(cfg->objs[0].metric_name),
             "avg_ready_wait");
    cfg->objs[0].direction = HAEUN_FB_DIR_LOWER_BETTER;
    cfg->objs[0].weight = 0.40;
    cfg->objs[0].improve_threshold = 0.05;
    cfg->objs[0].regress_threshold = 0.05;
    cfg->objs[0].min_participants = 1u;
    /* Prefer higher Jain fairness on run_ticks allocation. */
    snprintf(cfg->objs[1].metric_name, sizeof(cfg->objs[1].metric_name),
             "jain_fairness");
    cfg->objs[1].direction = HAEUN_FB_DIR_HIGHER_BETTER;
    cfg->objs[1].weight = 0.25;
    cfg->objs[1].improve_threshold = 0.02;
    cfg->objs[1].regress_threshold = 0.05;
    cfg->objs[1].min_participants = 2u;
    /* Prefer lower dispatch count as coarse overhead proxy. */
    snprintf(cfg->objs[2].metric_name, sizeof(cfg->objs[2].metric_name),
             "dispatch_count");
    cfg->objs[2].direction = HAEUN_FB_DIR_LOWER_BETTER;
    cfg->objs[2].weight = 0.20;
    cfg->objs[2].improve_threshold = 0.10;
    cfg->objs[2].regress_threshold = 0.15;
    cfg->objs[2].min_participants = 1u;
    /* Prefer higher completed count when available. */
    snprintf(cfg->objs[3].metric_name, sizeof(cfg->objs[3].metric_name),
             "completed");
    cfg->objs[3].direction = HAEUN_FB_DIR_HIGHER_BETTER;
    cfg->objs[3].weight = 0.15;
    cfg->objs[3].improve_threshold = 0.0;
    cfg->objs[3].regress_threshold = 0.0;
    cfg->objs[3].min_participants = 1u;
    cfg->obj_count = 4u;
    cfg->min_valid_objectives = 2u;
    cfg->consecutive_improve_needed = 2u;
    cfg->consecutive_regress_needed = 2u;
    cfg->cooldown_cycles = 1u;
    cfg->experimental = 1;
}

/**
 * Initializes controller state in SHADOW mode.
 */
void haeun_fb_state_init(haeun_fb_state *st) {
    if (st == NULL)
        return;
    memset(st, 0, sizeof(*st));
    st->schema_version = HAEUN_FB_SCHEMA;
    snprintf(st->controller_version, sizeof(st->controller_version),
             "r35-fb-1");
    st->mode = HAEUN_FB_MODE_SHADOW;
    st->previous_outcome = HAEUN_FB_INCONCLUSIVE;
    st->previous_outcome_valid = 0;
    st->confidence = HAEUN_PLAN_CONF_NONE;
    haeun_fb_config_default(&st->cfg);
}
