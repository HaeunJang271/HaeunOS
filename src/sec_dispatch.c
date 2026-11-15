#include "haeun/mmu.h"
#include "haeun/sec.h"

#include <stddef.h>

/**
 * Checks that a user virtual address is permitted for the access kind.
 */
haeun_sec_result haeun_sec_check_user_va(const haeun_sec_syscall_req *req,
                                         uint32_t va, uint32_t access_mask) {
    uint32_t pa = 0u;
    uint32_t cause = 0u;
    if (req == NULL) {
        haeun_sec_log("user_va", "null request");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    if (req->privilege == HAEUN_PRIV_SUPERVISOR) return HAEUN_SEC_OK;
    if (req->vm != NULL && (req->vm->csr_ptbr & 1u) != 0u) {
        /* Kernel may call this while still in S-mode; enforce U as the user. */
        uint32_t saved = req->vm->privilege;
        req->vm->privilege = HAEUN_PRIV_USER;
        if (!haeun_mmu_translate(req->vm, va, access_mask, &pa, &cause)) {
            req->vm->privilege = saved;
            haeun_sec_log("user_va", "MMU translate denied");
            return HAEUN_SEC_DENIED;
        }
        req->vm->privilege = saved;
        (void)pa;
        return HAEUN_SEC_OK;
    }
    return haeun_sec_check_user_range(req->privilege, va, 1u, req->user_lo,
                                      req->user_hi);
}

/**
 * Validates a syscall attempt without performing the side effects.
 */
haeun_sec_result haeun_sec_dispatch(const haeun_sec_syscall_req *req) {
    haeun_sec_result r;
    if (req == NULL) {
        haeun_sec_log("dispatch", "null request");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    r = haeun_sec_validate_syscall(req->privilege, req->number);
    if (r != HAEUN_SEC_OK) return r;
    switch (req->number) {
    case HAEUN_SYS_EXEC:
        if (req->exec_image != NULL || req->exec_size != 0u) {
            r = haeun_sec_validate_exec(req->exec_image, req->exec_size);
            if (r != HAEUN_SEC_OK) return r;
        }
        r = haeun_sec_check_user_va(req, req->arg0, HAEUN_MMU_ACCESS_X);
        if (r != HAEUN_SEC_OK) return r;
        break;
    case HAEUN_SYS_OPEN:
        /* r2==1 console device; otherwise r2 is a user path VA. */
        if (req->arg0 > 1u) {
            r = haeun_sec_check_user_va(req, req->arg0, HAEUN_MMU_ACCESS_R);
            if (r != HAEUN_SEC_OK) return r;
        }
        break;
    case HAEUN_SYS_WRITE:
    case HAEUN_SYS_READ:
    case HAEUN_SYS_CLOSE:
        break;
    case HAEUN_SYS_SOCKET:
    case HAEUN_SYS_BIND:
    case HAEUN_SYS_SENDTO:
    case HAEUN_SYS_RECVFROM:
    case HAEUN_SYS_LISTEN:
    case HAEUN_SYS_ACCEPT:
    case HAEUN_SYS_SO_CLOSE:
        /* Buffer VAs for SENDTO/RECVFROM are checked in sock ECALL assist. */
        break;
    case HAEUN_SYS_WIN_CREATE:
    case HAEUN_SYS_WIN_DESTROY:
    case HAEUN_SYS_WIN_FILL:
    case HAEUN_SYS_WIN_PIXEL:
    case HAEUN_SYS_WIN_RAISE:
    case HAEUN_SYS_GFX_COMPOSE:
        break;
    case HAEUN_SYS_APP_COUNT:
    case HAEUN_SYS_APP_NAME:
    case HAEUN_SYS_APP_SPAWN:
    case HAEUN_SYS_APP_INFO:
    case HAEUN_SYS_APP_RUN:
    case HAEUN_SYS_APP_CTRL:
        /* Name/info buffer VAs are checked in the app ECALL assist. */
        break;
    case HAEUN_SYS_SERVICE_COUNT:
    case HAEUN_SYS_SERVICE_INFO:
    case HAEUN_SYS_SERVICE_START:
    case HAEUN_SYS_SERVICE_STOP:
    case HAEUN_SYS_SERVICE_STATUS:
    case HAEUN_SYS_SERVICE_RESTART:
        break;
    case HAEUN_SYS_FS_READDIR:
    case HAEUN_SYS_FS_UNLINK:
    case HAEUN_SYS_FS_STAT:
    case HAEUN_SYS_FS_SEEK:
        break;
    case HAEUN_SYS_EVENT_POLL:
    case HAEUN_SYS_WIN_DRAW_TEXT:
    case HAEUN_SYS_IPC_CREATE:
    case HAEUN_SYS_IPC_SEND:
    case HAEUN_SYS_IPC_RECV:
    case HAEUN_SYS_IPC_CLOSE:
    case HAEUN_SYS_SVC_REGISTER:
    case HAEUN_SYS_SVC_LOOKUP:
    case HAEUN_SYS_IPC_AVAILABLE:
    case HAEUN_SYS_IPC_PEEK:
    case HAEUN_SYS_WIN_QUERY:
        /* Pointer checks performed in m20_ecall assist. */
        break;
    case HAEUN_SYS_EXIT:
    case HAEUN_SYS_FORK:
    case HAEUN_SYS_WAIT:
    case HAEUN_SYS_SLEEP:
    case HAEUN_SYS_GETPID:
    case HAEUN_SYS_YIELD:
        break;
    default:
        haeun_sec_log("dispatch", "unreachable");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}
