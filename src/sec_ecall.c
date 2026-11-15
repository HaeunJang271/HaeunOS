#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stddef.h>

/**
 * Builds a dispatch request from the live user GPRs (r1=number, r2/r3=args).
 */
haeun_sec_result haeun_sec_on_ecall(haeun_vm *vm) {
    haeun_sec_syscall_req req;
    if (vm == NULL) {
        haeun_sec_log("ecall", "null vm");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    req.privilege = vm->privilege;
    req.number = haeun_rf_read(vm, 1u);
    req.arg0 = haeun_rf_read(vm, 2u);
    req.arg1 = haeun_rf_read(vm, 3u);
    req.vm = vm;
    req.user_lo = 0u;
    req.user_hi = (uint32_t)vm->ram_size;
    req.exec_image = NULL;
    req.exec_size = 0u;
    return haeun_sec_dispatch(&req);
}

/**
 * User ECALL pre-check: on deny, poisons r1 so guest trap returns -1.
 * Always returns whether the trap should still be delivered (always true).
 */
bool haeun_sec_ecall_before_trap(haeun_vm *vm) {
    if (vm == NULL) return true;
    if (vm->privilege != HAEUN_PRIV_USER) return true;
    if (haeun_sec_on_ecall(vm) != HAEUN_SEC_OK) {
        haeun_rf_write(vm, 1u, 0xffffffffu);
        haeun_sec_log("ecall", "poisoned syscall number for guest deny");
    }
    return true;
}
