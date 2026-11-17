#ifndef HAEUN_PVAL_VALIDATE_H
#define HAEUN_PVAL_VALIDATE_H

#include "haeun/pval_report.h"
#include "haeun/vm.h"

/**
 * Full read-only validation. target_policy UINT32_MAX skips compatibility.
 * Does not step the VM or write guest memory.
 */
haeun_pval_result haeun_pval_validate(const haeun_vm *vm, uint32_t target_policy,
                                      haeun_pval_report *out);

#endif
