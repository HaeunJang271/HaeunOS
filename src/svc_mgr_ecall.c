#include "haeun/guest_pcb.h"
#include "haeun/mmu.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
#include "haeun/svc_mgr.h"

#include <string.h>

static int copy_out(haeun_vm *vm, uint32_t va, uint32_t cap, const char *s) {
    uint32_t i;
    haeun_sec_syscall_req sreq;
    if (cap == 0u || cap > HAEUN_SM_DESC_MAX)
        return -1;
    memset(&sreq, 0, sizeof(sreq));
    sreq.privilege = HAEUN_PRIV_USER;
    sreq.vm = vm;
    sreq.user_hi = (uint32_t)vm->ram_size;
    if (haeun_sec_check_user_va(&sreq, va, HAEUN_MMU_ACCESS_W) != HAEUN_SEC_OK)
        return -1;
    for (i = 0u; i < cap; ++i) {
        uint8_t b = (i < strlen(s)) ? (uint8_t)s[i] : 0u;
        uint32_t pa = 0u;
        uint32_t cause = 0u;
        if ((vm->csr_ptbr & 1u) != 0u) {
            if (!haeun_mmu_translate(vm, va + i, HAEUN_MMU_ACCESS_W, &pa,
                                     &cause) ||
                pa >= vm->ram_size)
                return -1;
            vm->ram[pa] = b;
        } else if (va + i >= vm->ram_size) {
            return -1;
        } else {
            vm->ram[va + i] = b;
        }
        if (b == 0u)
            break;
    }
    return 0;
}

/**
 * Host ECALL assist for SERVICE_* (41–46).
 */
bool haeun_sm_ecall_io(haeun_vm *vm, void *context) {
    haeun_sm_hub *hub = (haeun_sm_hub *)context;
    uint32_t number, a0, a1, a2, a3;
    char buf[HAEUN_SM_DESC_MAX];
    uint32_t flags = 0u;
    uint32_t pid;
    haeun_sm_stat st;
    if (vm == NULL || hub == NULL || vm->privilege != HAEUN_PRIV_USER)
        return false;
    number = haeun_rf_read(vm, 1u);
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    a3 = haeun_rf_read(vm, 5u);
    pid = haeun_guest_pcb_current_pid(vm);
    switch (number) {
    case HAEUN_SYS_SERVICE_COUNT:
        haeun_rf_write(vm, 1u, (uint32_t)haeun_sm_count(hub));
        return true;
    case HAEUN_SYS_SERVICE_INFO:
        if ((int)a1 == HAEUN_SM_FIELD_FLAGS) {
            if (haeun_sm_info(hub, (int)a0, (int)a1, NULL, 0u, &flags) !=
                HAEUN_SM_OK) {
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
            haeun_rf_write(vm, 1u, 0u);
            haeun_rf_write(vm, 2u, flags);
            return true;
        }
        if (haeun_sm_info(hub, (int)a0, (int)a1, buf, sizeof(buf), NULL) !=
                HAEUN_SM_OK ||
            copy_out(vm, a2, a3, buf) != 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    case HAEUN_SYS_SERVICE_START:
        if (haeun_sm_start(hub, (int)a0) != HAEUN_SM_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, hub->tab[a0].pid);
        return true;
    case HAEUN_SYS_SERVICE_STOP:
        if (haeun_sm_stop(hub, (int)a0, pid) != HAEUN_SM_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    case HAEUN_SYS_SERVICE_STATUS:
        if (haeun_sm_status(hub, (int)a0, &st) != HAEUN_SM_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        haeun_rf_write(vm, 2u, st.state);
        haeun_rf_write(vm, 3u, st.pid);
        haeun_rf_write(vm, 4u, st.last_exit | (st.run_count << 16));
        haeun_rf_write(vm, 5u, st.fail_count | (st.restart_count << 16));
        return true;
    case HAEUN_SYS_SERVICE_RESTART:
        if (haeun_sm_restart(hub, (int)a0, pid) != HAEUN_SM_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, hub->tab[a0].pid);
        return true;
    default:
        return false;
    }
}
