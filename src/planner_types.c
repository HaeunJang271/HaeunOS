#include "haeun/planner_types.h"

#include <string.h>

/**
 * Fills conservative experimental defaults.
 */
void haeun_plan_config_default(haeun_plan_config *cfg) {
    if (cfg == NULL)
        return;
    memset(cfg, 0, sizeof(*cfg));
    /* Experimental thresholds — not proven optima; see docs/r33-decision-model.md */
    cfg->ready_run_ratio_high = 2.0;
    cfg->sleep_share_high = 0.45;
    cfg->dispatch_run_high = 0.15;
    cfg->min_dwell_ticks = 1000u;
    cfg->cooldown_ticks = 500u;
    cfg->consecutive_needed = 2u;
    cfg->hysteresis_delta = 0.25;
    cfg->plan_ttl_ticks = 100000u;
    cfg->experimental_rules = 1;
}

/**
 * Initializes session with defaults.
 */
void haeun_plan_session_init(haeun_plan_session *sess) {
    if (sess == NULL)
        return;
    memset(sess, 0, sizeof(*sess));
    haeun_plan_config_default(&sess->cfg);
    sess->last_policy = UINT32_MAX;
}
