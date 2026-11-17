#include "haeun/pval_timestamp.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { ST_READY = 1, ST_RUNNING = 2 };

static const uint32_t TS_HALF = UINT32_C(0x80000000);

static void add(haeun_pval_timestamp_report *out, const char *id,
                haeun_pval_status st, const char *why) {
    haeun_pval_check *c;
    if (out->check_count >= 8u)
        return;
    c = &out->checks[out->check_count++];
    snprintf(c->id, sizeof(c->id), "%s", id);
    c->status = st;
    snprintf(c->explanation, sizeof(c->explanation), "%s", why);
}

/**
 * Validates ready_since pairwise modular ordering assumptions for FIFO.
 */
haeun_pval_result haeun_pval_timestamp_check(const haeun_pval_snapshot *snap,
                                             haeun_pval_timestamp_report *out) {
    uint32_t rs[HAEUN_PVAL_AS_SLOTS];
    unsigned n = 0u;
    unsigned i;
    unsigned j;
    if (snap == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    for (i = 0; i < HAEUN_PVAL_AS_SLOTS; ++i) {
        const haeun_pval_slot *s = &snap->slots[i];
        if (!s->as_slot_valid)
            continue;
        if (s->state != ST_READY && s->state != ST_RUNNING)
            continue;
        rs[n++] = s->ready_since;
    }
    out->eligible_count = n;
    out->external_bound_proven = 0;
    if (n == 0u) {
        add(out, "TS.empty", HAEUN_PVAL_NOT_APPLICABLE, "no READY/RUNNING candidates");
        out->pairwise_comparable = 1;
        out->overall = HAEUN_PVAL_NOT_APPLICABLE;
        return HAEUN_PVAL_OK;
    }
    if (n == 1u) {
        add(out, "TS.single", HAEUN_PVAL_PASS, "single eligible candidate");
        out->pairwise_comparable = 1;
        add(out, "TS.external_bound", HAEUN_PVAL_UNKNOWN,
            "lifetime READY span <2^31 not proven by snapshot");
        out->overall = HAEUN_PVAL_UNKNOWN;
        return HAEUN_PVAL_OK;
    }
    for (i = 0; i < n; ++i) {
        for (j = i + 1u; j < n; ++j) {
            uint32_t d = rs[i] - rs[j];
            if (d == TS_HALF) {
                out->exact_half_range_pair = 1;
                add(out, "TS.half_range", HAEUN_PVAL_FAIL,
                    "pair separated by exactly 2^31; ordering ambiguous");
                out->pairwise_comparable = 0;
                add(out, "TS.external_bound", HAEUN_PVAL_UNKNOWN,
                    "lifetime READY span <2^31 not proven by snapshot");
                out->overall = HAEUN_PVAL_FAIL;
                return HAEUN_PVAL_OK;
            }
        }
    }
    out->pairwise_comparable = 1;
    add(out, "TS.pairwise", HAEUN_PVAL_PASS,
        "no eligible pair at exact 2^31 separation");
    add(out, "TS.external_bound", HAEUN_PVAL_UNKNOWN,
        "lifetime READY span <2^31 not proven by snapshot");
    out->overall = HAEUN_PVAL_UNKNOWN;
    return HAEUN_PVAL_OK;
}
