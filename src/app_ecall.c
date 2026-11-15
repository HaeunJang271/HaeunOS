#include "haeun/app_env.h"
#include "haeun/guest_pcb.h"
#include "haeun/mmu.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <string.h>

static int copy_out(haeun_vm *vm, uint32_t va, uint32_t cap, const char *name) {
    uint32_t i;
    haeun_sec_syscall_req sreq;
    if (cap == 0u || cap >= HAEUN_APP_DESC_MAX + 1u)
        return -1;
    memset(&sreq, 0, sizeof(sreq));
    sreq.privilege = HAEUN_PRIV_USER;
    sreq.vm = vm;
    sreq.user_hi = (uint32_t)vm->ram_size;
    if (haeun_sec_check_user_va(&sreq, va, HAEUN_MMU_ACCESS_W) != HAEUN_SEC_OK)
        return -1;
    for (i = 0u; i < cap; ++i) {
        uint8_t b = (i < strlen(name)) ? (uint8_t)name[i] : 0u;
        uint32_t pa = 0u;
        uint32_t cause = 0u;
        if ((vm->csr_ptbr & 1u) != 0u) {
            if (!haeun_mmu_translate(vm, va + i, HAEUN_MMU_ACCESS_W, &pa,
                                     &cause) ||
                pa >= vm->ram_size)
                return -1;
            vm->ram[pa] = b;
        } else {
            if (va + i >= vm->ram_size)
                return -1;
            vm->ram[va + i] = b;
        }
        if (b == 0u)
            break;
    }
    return 0;
}

/**
 * Host ECALL assist for APP_COUNT / APP_NAME / APP_SPAWN / INFO / RUN / CTRL.
 */
bool haeun_app_ecall_io(haeun_vm *vm, void *context) {
    haeun_app_env *env = (haeun_app_env *)context;
    uint32_t number;
    uint32_t a0, a1, a2, a3;
    char buf[HAEUN_APP_DESC_MAX];
    uint32_t pid = 0u;
    uint32_t flags = 0u;
    haeun_app_run_info ri;
    if (vm == NULL || env == NULL)
        return false;
    if (vm->privilege != HAEUN_PRIV_USER)
        return false;
    number = haeun_rf_read(vm, 1u);
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    a3 = haeun_rf_read(vm, 5u);
    switch (number) {
    case HAEUN_SYS_APP_COUNT:
        haeun_rf_write(vm, 1u, (uint32_t)haeun_app_count(env));
        return true;
    case HAEUN_SYS_APP_NAME:
        if (a2 == 0u || a2 >= HAEUN_APP_NAME_MAX) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (haeun_app_name(env, (int)a0, buf, sizeof(buf)) != HAEUN_APP_OK ||
            copy_out(vm, a1, a2, buf) != 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    case HAEUN_SYS_APP_SPAWN:
        if (haeun_app_spawn(env, (int)a0, &pid) != HAEUN_APP_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, pid);
        return true;
    case HAEUN_SYS_APP_INFO:
        if ((int)a1 == HAEUN_APP_FIELD_FLAGS) {
            if (haeun_app_info(env, (int)a0, (int)a1, NULL, 0u, &flags) !=
                HAEUN_APP_OK) {
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
            haeun_rf_write(vm, 1u, 0u);
            haeun_rf_write(vm, 2u, flags);
            return true;
        }
        if (a3 == 0u || a3 > HAEUN_APP_DESC_MAX) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (haeun_app_info(env, (int)a0, (int)a1, buf, sizeof(buf), NULL) !=
                HAEUN_APP_OK ||
            copy_out(vm, a2, a3, buf) != 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    case HAEUN_SYS_APP_RUN:
        if (haeun_app_run_at(env, (int)a0, &ri) != HAEUN_APP_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        haeun_rf_write(vm, 2u, ri.pid);
        haeun_rf_write(vm, 3u, (uint32_t)ri.app_index);
        haeun_rf_write(vm, 4u, ri.state | ((ri.exit_code & 0xffffu) << 16));
        haeun_rf_write(vm, 5u, ri.win_id < 0 ? 0xffffffffu : (uint32_t)ri.win_id);
        return true;
    case HAEUN_SYS_APP_CTRL:
        pid = haeun_guest_pcb_current_pid(vm);
        if (haeun_app_ctrl(env, (int)a0, a1, pid) != HAEUN_APP_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    default:
        return false;
    }
}
