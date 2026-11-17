#ifndef HAEUN_RECONFIG_CLI_H
#define HAEUN_RECONFIG_CLI_H

#include "haeun/vm.h"

#include <stdint.h>

/**
 * Runs request→prepare→commit for source→target on a live VM.
 * Returns 0 on COMMITTED, 1 on reject/fail, 2 on bad args.
 */
int haeun_recfg_cli_run(haeun_vm *vm, uint32_t source_policy,
                        uint32_t target_policy, int json);

/**
 * Parses policy name "rr" or "fcfs". Returns -1 if unknown.
 */
int haeun_recfg_cli_parse_policy(const char *name, uint32_t *out);

#endif
