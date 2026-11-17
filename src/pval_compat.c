#include "haeun/pval_compat.h"

#include <stdio.h>
#include <string.h>

static void add(haeun_pval_compat_report *out, const char *id,
                haeun_pval_status st, const char *why) {
    haeun_pval_check *c;
    if (out->check_count >= 16u)
        return;
    c = &out->checks[out->check_count++];
    snprintf(c->id, sizeof(c->id), "%s", id);
    c->status = st;
    snprintf(c->explanation, sizeof(c->explanation), "%s", why);
}

static int known(uint32_t id) {
    return id == HAEUN_PVAL_POLICY_RR || id == HAEUN_PVAL_POLICY_FCFS;
}

/**
 * Evaluates transition preconditions without performing a transition.
 */
haeun_pval_result haeun_pval_compat_check(const haeun_pval_snapshot *snap,
                                          uint32_t target_policy,
                                          const haeun_pval_timestamp_report *ts,
                                          haeun_pval_compat_report *out) {
    int need_ts;
    if (snap == NULL || ts == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->source_policy = snap->policy_id;
    out->target_policy = target_policy;
    if (!known(snap->policy_id)) {
        add(out, "CMP.source", HAEUN_PVAL_FAIL, "source policy unrecognized");
        out->overall = HAEUN_PVAL_FAIL;
        return HAEUN_PVAL_OK;
    }
    add(out, "CMP.source", HAEUN_PVAL_PASS, "source policy recognized");
    if (!known(target_policy)) {
        add(out, "CMP.target", HAEUN_PVAL_FAIL, "target policy unrecognized");
        out->overall = HAEUN_PVAL_FAIL;
        return HAEUN_PVAL_OK;
    }
    add(out, "CMP.target", HAEUN_PVAL_PASS, "target policy recognized");
    add(out, "CMP.slots", HAEUN_PVAL_PASS, "AS slot model 0..2 assumed");
    need_ts = (target_policy == HAEUN_PVAL_POLICY_FCFS);
    if (!need_ts) {
        add(out, "CMP.timestamp", HAEUN_PVAL_NOT_APPLICABLE,
            "target RR does not require FIFO timestamp ordering");
    } else if (ts->overall == HAEUN_PVAL_FAIL) {
        add(out, "CMP.timestamp", HAEUN_PVAL_FAIL,
            "FIFO timestamp assumptions failed for this snapshot");
    } else if (ts->overall == HAEUN_PVAL_UNKNOWN) {
        add(out, "CMP.timestamp", HAEUN_PVAL_UNKNOWN,
            "pairwise OK or trivial, but lifetime <2^31 unbound");
    } else if (ts->overall == HAEUN_PVAL_NOT_APPLICABLE) {
        add(out, "CMP.timestamp", HAEUN_PVAL_PASS,
            "no eligible candidates; FIFO ordering vacuous");
    } else {
        add(out, "CMP.timestamp", ts->overall, "timestamp report propagated");
    }
    out->overall = HAEUN_PVAL_PASS;
    {
        unsigned i;
        for (i = 0; i < out->check_count; ++i) {
            haeun_pval_status s = out->checks[i].status;
            if (s == HAEUN_PVAL_FAIL) {
                out->overall = HAEUN_PVAL_FAIL;
                break;
            }
            if (s == HAEUN_PVAL_UNKNOWN)
                out->overall = HAEUN_PVAL_UNKNOWN;
            else if (s == HAEUN_PVAL_WARN && out->overall == HAEUN_PVAL_PASS)
                out->overall = HAEUN_PVAL_WARN;
        }
    }
    out->transition_preconditions_satisfied =
        (out->overall == HAEUN_PVAL_PASS) ? 1 : 0;
    return HAEUN_PVAL_OK;
}
