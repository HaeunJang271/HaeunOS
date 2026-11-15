#ifndef HAEUN_GUEST_PCB_H
#define HAEUN_GUEST_PCB_H

#include "haeun/vm.h"

#include <stdint.h>

enum {
    HAEUN_GUEST_PCB_NPROC = 4,   /* PCB table stride count in guest RAM */
    HAEUN_GUEST_PCB_AS_SLOTS = 3 /* distinct page-table / stack slots */
};

/**
 * Reads the current process pid from the process-v1 PCB table in guest RAM.
 * Returns 0 when the PCB region is missing or invalid.
 */
uint32_t haeun_guest_pcb_current_pid(const haeun_vm *vm);

#endif
