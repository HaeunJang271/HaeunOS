#include "haeun/pval_invariants.h"

#include <stdio.h>
#include <string.h>

enum {
    ST_FREE = 0,
    ST_READY = 1,
    ST_RUNNING = 2,
    ST_SLEEP = 3,
    ST_WAIT = 4,
    ST_ZOMBIE = 5
};

static void add(haeun_pval_invariant_report *out, const char *id,
                haeun_pval_status st, const char *why) {
    haeun_pval_check *c;
    if (out->count >= HAEUN_PVAL_CHECK_MAX)
        return;
    c = &out->checks[out->count++];
    snprintf(c->id, sizeof(c->id), "%s", id);
    c->status = st;
    snprintf(c->explanation, sizeof(c->explanation), "%s", why);
}

static haeun_pval_status fold(const haeun_pval_invariant_report *r) {
    unsigned i;
    haeun_pval_status o = HAEUN_PVAL_PASS;
    for (i = 0; i < r->count; ++i) {
        haeun_pval_status s = r->checks[i].status;
        if (s == HAEUN_PVAL_FAIL)
            return HAEUN_PVAL_FAIL;
        if (s == HAEUN_PVAL_UNKNOWN && o != HAEUN_PVAL_FAIL)
            o = HAEUN_PVAL_UNKNOWN;
        else if (s == HAEUN_PVAL_WARN && o == HAEUN_PVAL_PASS)
            o = HAEUN_PVAL_WARN;
    }
    return o;
}

/**
 * Runs PCB/current/policy/telemetry invariant checks on a snapshot.
 */
haeun_pval_result haeun_pval_invariants_check(const haeun_pval_snapshot *snap,
                                              haeun_pval_invariant_report *out) {
    unsigned i;
    unsigned running = 0u;
    if (snap == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    /* Policy */
    if (snap->policy_id == HAEUN_PVAL_POLICY_RR ||
        snap->policy_id == HAEUN_PVAL_POLICY_FCFS)
        add(out, "POL.recognized", HAEUN_PVAL_PASS, "active policy ID known");
    else
        add(out, "POL.recognized", HAEUN_PVAL_FAIL, "unknown active policy ID");
    /* Slot 3 */
    if (!snap->slots[3].as_slot_valid && !snap->slots[3].is_participant)
        add(out, "PCB.slot3_excluded", HAEUN_PVAL_PASS, "slot 3 not a participant");
    else
        add(out, "PCB.slot3_excluded", HAEUN_PVAL_FAIL, "slot 3 treated as participant");
    /* PCB states */
    for (i = 0; i < HAEUN_PVAL_AS_SLOTS; ++i) {
        const haeun_pval_slot *s = &snap->slots[i];
        if (s->state > ST_ZOMBIE) {
            add(out, "PCB.state_range", HAEUN_PVAL_FAIL, "state out of 0..5");
            break;
        }
        if (i == HAEUN_PVAL_AS_SLOTS - 1u)
            add(out, "PCB.state_range", HAEUN_PVAL_PASS, "AS slot states in 0..5");
        if (s->state == ST_RUNNING)
            running++;
        if (s->is_participant && s->as_slot_valid == 0)
            add(out, "PCB.as_consistency", HAEUN_PVAL_FAIL,
                "participant without AS");
        if (s->telem_kind == HAEUN_PVAL_FIELD_STALE)
            add(out, "TELEM.stale_free", HAEUN_PVAL_WARN,
                "FREE slot retains stale telemetry (expected)");
    }
    if (running > 1u)
        add(out, "CUR.single_running", HAEUN_PVAL_FAIL, "more than one RUNNING");
    else
        add(out, "CUR.single_running", HAEUN_PVAL_PASS, "at most one RUNNING");
    /* curr */
    if (snap->idle_flag != 0u) {
        add(out, "CUR.idle_semantics", HAEUN_PVAL_PASS,
            "IDLE_FLAG set; RUNNING not required");
    } else if (snap->curr_index >= HAEUN_PVAL_AS_SLOTS) {
        add(out, "CUR.index_range", HAEUN_PVAL_FAIL, "curr outside AS slots");
    } else {
        const haeun_pval_slot *c = &snap->slots[snap->curr_index];
        add(out, "CUR.index_range", HAEUN_PVAL_PASS, "curr in 0..2");
        if (c->state == ST_RUNNING)
            add(out, "CUR.running_match", HAEUN_PVAL_PASS,
                "curr slot is RUNNING");
        else if (c->state == ST_ZOMBIE || c->state == ST_FREE)
            add(out, "CUR.running_match", HAEUN_PVAL_WARN,
                "curr not RUNNING (exit/idle edge)");
        else
            add(out, "CUR.running_match", HAEUN_PVAL_WARN,
                "curr not RUNNING while idle_flag clear");
    }
    /* Completion */
    for (i = 0; i < HAEUN_PVAL_AS_SLOTS; ++i) {
        const haeun_pval_slot *s = &snap->slots[i];
        if (s->complete_valid != 0u && s->state != ST_ZOMBIE &&
            s->state != ST_FREE) {
            add(out, "TELEM.complete_state", HAEUN_PVAL_WARN,
                "complete_valid with non-ZOMBIE/FREE state");
            break;
        }
        if (i == HAEUN_PVAL_AS_SLOTS - 1u)
            add(out, "TELEM.complete_state", HAEUN_PVAL_PASS,
                "completion flags consistent or absent");
    }
    out->overall = fold(out);
    return HAEUN_PVAL_OK;
}
