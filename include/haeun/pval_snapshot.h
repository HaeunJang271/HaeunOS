#ifndef HAEUN_PVAL_SNAPSHOT_H
#define HAEUN_PVAL_SNAPSHOT_H

#include "haeun/pval_types.h"
#include "haeun/vm.h"

/**
 * Reads a const snapshot from the VM. Does not write guest RAM or step the VM.
 */
haeun_pval_result haeun_pval_snapshot_read(const haeun_vm *vm,
                                           haeun_pval_snapshot *out);

/**
 * Recomputes the seal from the live VM and compares to snap->seal.
 * Returns HAEUN_PVAL_MUTATION on mismatch.
 */
haeun_pval_result haeun_pval_snapshot_verify_unchanged(const haeun_vm *vm,
                                                       const haeun_pval_snapshot *snap);

#endif
