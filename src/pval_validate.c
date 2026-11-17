#include "haeun/pval_validate.h"

#include "haeun/pval_snapshot.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Full read-only validation. target_policy UINT32_MAX skips compatibility.
 */
haeun_pval_result haeun_pval_validate(const haeun_vm *vm, uint32_t target_policy,
                                      haeun_pval_report *out) {
    haeun_pval_status parts[4];
    unsigned n = 0u;
    haeun_pval_result er;
    if (vm == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    snprintf(out->validation_id, sizeof(out->validation_id), "r31-pval");
    er = haeun_pval_snapshot_read(vm, &out->snap);
    if (er != HAEUN_PVAL_OK)
        return er;
    (void)haeun_pval_invariants_check(&out->snap, &out->invariants);
    (void)haeun_pval_timestamp_check(&out->snap, &out->timestamps);
    if (target_policy != UINT32_MAX) {
        (void)haeun_pval_compat_check(&out->snap, target_policy, &out->timestamps,
                                      &out->compat);
        out->compat_ran = 1;
    }
    er = haeun_pval_snapshot_verify_unchanged(vm, &out->snap);
    if (er == HAEUN_PVAL_MUTATION) {
        out->read_only = HAEUN_PVAL_FAIL;
        out->overall = HAEUN_PVAL_FAIL;
        return HAEUN_PVAL_MUTATION;
    }
    if (er != HAEUN_PVAL_OK)
        return er;
    out->read_only = HAEUN_PVAL_PASS;
    parts[n++] = out->read_only;
    parts[n++] = out->invariants.overall;
    parts[n++] = out->timestamps.overall;
    if (out->compat_ran)
        parts[n++] = out->compat.overall;
    out->overall = haeun_pval_aggregate(parts, n);
    fprintf(stderr, "haeun-pval: validate overall=%d\n", (int)out->overall);
    return HAEUN_PVAL_OK;
}
