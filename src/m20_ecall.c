#include "haeun/m20_ecall.h"
#include "haeun/gfx_ui.h"
#include "haeun/guest_pcb.h"
#include "haeun/mmu.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stdio.h>
#include <string.h>

static void m20_log(const char *detail) {
    (void)fprintf(stderr, "[haeun-m20] failure: %s\n",
                  detail != NULL ? detail : "");
}

static int cur_pid(haeun_vm *vm) {
    return (int)haeun_guest_pcb_current_pid(vm);
}

static bool guest_load_u8(haeun_vm *vm, uint32_t va, uint8_t *out) {
    uint32_t pa = 0u, cause = 0u;
    if (vm == NULL || out == NULL)
        return false;
    if ((vm->csr_ptbr & 1u) != 0u) {
        if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_R, &pa, &cause) ||
            pa >= vm->ram_size)
            return false;
        *out = vm->ram[pa];
        return true;
    }
    if (va >= vm->ram_size)
        return false;
    *out = vm->ram[va];
    return true;
}

static bool guest_store_u8(haeun_vm *vm, uint32_t va, uint8_t b) {
    uint32_t pa = 0u, cause = 0u;
    if (vm == NULL)
        return false;
    if ((vm->csr_ptbr & 1u) != 0u) {
        if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_W, &pa, &cause) ||
            pa >= vm->ram_size)
            return false;
        vm->ram[pa] = b;
        return true;
    }
    if (va >= vm->ram_size)
        return false;
    vm->ram[va] = b;
    return true;
}

static bool copy_guest_str(haeun_vm *vm, uint32_t va, char *out, size_t cap) {
    size_t i;
    if (cap == 0u)
        return false;
    for (i = 0; i + 1u < cap; ++i) {
        uint8_t b = 0u;
        if (!guest_load_u8(vm, va + (uint32_t)i, &b))
            return false;
        out[i] = (char)b;
        if (b == 0u)
            return true;
    }
    out[cap - 1u] = '\0';
    return true;
}

static int deny_win(haeun_gfx_wm *wm, int pid, int id) {
    if (pid == 0)
        return 0;
    if (!haeun_gfx_wm_owned_by(wm, id, pid)) {
        m20_log("window ownership denied");
        return 1;
    }
    return 0;
}

/**
 * Host assist for EVENT_POLL, WIN_DRAW_TEXT, IPC_*, SVC_*, AVAILABLE/PEEK (27–36).
 */
bool haeun_m20_ecall_io(haeun_vm *vm, void *context) {
    haeun_m20_ecall_ctx *ctx = (haeun_m20_ecall_ctx *)context;
    uint32_t number, a0, a1, a2, a3, a4, a5;
    int pid;
    haeun_evt ev;
    char text[HAEUN_GFX_UI_TEXT_MAX];
    uint8_t buf[HAEUN_IPC_MSG_MAX];
    int n, sender = 0;
    haeun_sec_syscall_req sreq;
    if (vm == NULL || ctx == NULL)
        return false;
    if (vm->privilege != HAEUN_PRIV_USER)
        return false;
    number = haeun_rf_read(vm, 1u);
    if (number < (uint32_t)HAEUN_SYS_EVENT_POLL ||
        number > (uint32_t)HAEUN_SYS_WIN_QUERY)
        return false;
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    a3 = haeun_rf_read(vm, 5u);
    a4 = haeun_rf_read(vm, 6u);
    a5 = haeun_rf_read(vm, 7u);
    pid = cur_pid(vm);
    switch (number) {
    case HAEUN_SYS_EVENT_POLL:
        if (ctx->evt == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0u);
            return true;
        }
        if (!haeun_evt_poll(ctx->evt, pid, &ev)) {
            haeun_rf_write(vm, 1u, 0u);
            return true;
        }
        haeun_rf_write(vm, 1u, (uint32_t)ev.type);
        haeun_rf_write(vm, 2u, (uint32_t)(int32_t)ev.win);
        haeun_rf_write(vm, 3u, ev.a);
        haeun_rf_write(vm, 4u, ev.b);
        return true;
    case HAEUN_SYS_WIN_DRAW_TEXT: {
        size_t ti;
        uint8_t b = 0u;
        if (ctx->wm == NULL || ctx->gfx == NULL) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (deny_win(ctx->wm, pid, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if ((int32_t)a1 < 0 || (int32_t)a2 < 0) {
            m20_log("draw_text negative origin");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (haeun_sec_check_user_va(&sreq, a5, HAEUN_MMU_ACCESS_R) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        /* Strict length: reject if no NUL within TEXT_MAX bytes. */
        for (ti = 0; ti < sizeof(text); ++ti) {
            if (!guest_load_u8(vm, a5 + (uint32_t)ti, &b)) {
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
            if (b == 0u) {
                text[ti] = '\0';
                break;
            }
            if (ti + 1u >= sizeof(text)) {
                m20_log("draw_text string too long");
                haeun_rf_write(vm, 1u, 0xfffffffeu);
                return true;
            }
            text[ti] = (char)b;
        }
        if (haeun_gfx_ui_draw_text(ctx->wm, (int)a0, a1, a2, text, a3, a4) !=
            HAEUN_GFX_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        return true;
    }
    case HAEUN_SYS_IPC_CREATE:
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_ipc_create(ctx->ipc, pid);
        haeun_rf_write(vm, 1u, n < 0 ? 0xffffffffu : (uint32_t)n);
        return true;
    case HAEUN_SYS_IPC_SEND:
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (a2 == 0u || a2 > (uint32_t)HAEUN_IPC_MSG_MAX) {
            haeun_rf_write(vm, 1u, 0xfffffffeu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (haeun_sec_check_user_va(&sreq, a1, HAEUN_MMU_ACCESS_R) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        for (n = 0; n < (int)a2; ++n) {
            if (!guest_load_u8(vm, a1 + (uint32_t)n, &buf[n])) {
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
        }
        n = haeun_ipc_send(ctx->ipc, (int)a0, pid, buf, a2);
        haeun_rf_write(vm, 1u, n == 0 ? 0u : (n == -2 ? 0xfffffffdu : 0xffffffffu));
        return true;
    case HAEUN_SYS_IPC_RECV:
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (a2 == 0u || a2 > (uint32_t)HAEUN_IPC_MSG_MAX) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (haeun_sec_check_user_va(&sreq, a1, HAEUN_MMU_ACCESS_W) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_ipc_recv(ctx->ipc, (int)a0, pid, buf, a2, &sender);
        if (n < 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (n == 0) {
            haeun_rf_write(vm, 1u, 0u);
            return true;
        }
        for (a3 = 0u; a3 < (uint32_t)n; ++a3) {
            if (!guest_store_u8(vm, a1 + a3, buf[a3])) {
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
        }
        haeun_rf_write(vm, 1u, (uint32_t)n);
        haeun_rf_write(vm, 2u, (uint32_t)sender);
        return true;
    case HAEUN_SYS_IPC_CLOSE:
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_ipc_close(ctx->ipc, (int)a0, pid);
        haeun_rf_write(vm, 1u, n == 0 ? 0u : 0xffffffffu);
        return true;
    case HAEUN_SYS_SVC_REGISTER:
        if (ctx->svc == NULL || ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (haeun_sec_check_user_va(&sreq, a0, HAEUN_MMU_ACCESS_R) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (!copy_guest_str(vm, a0, text, HAEUN_SVC_NAME)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_svc_register(ctx->svc, pid, text, (int)a1);
        haeun_rf_write(vm, 1u, n == 0 ? 0u : 0xffffffffu);
        return true;
    case HAEUN_SYS_SVC_LOOKUP:
        if (ctx->svc == NULL) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (haeun_sec_check_user_va(&sreq, a0, HAEUN_MMU_ACCESS_R) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (!copy_guest_str(vm, a0, text, HAEUN_SVC_NAME)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_svc_lookup(ctx->svc, text);
        haeun_rf_write(vm, 1u, n < 0 ? 0xffffffffu : (uint32_t)n);
        return true;
    case HAEUN_SYS_IPC_AVAILABLE:
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_ipc_available(ctx->ipc, (int)a0, pid);
        haeun_rf_write(vm, 1u, n < 0 ? 0xffffffffu : (uint32_t)n);
        return true;
    case HAEUN_SYS_IPC_PEEK: {
        haeun_ipc_msg peek;
        if (ctx->ipc == NULL || pid <= 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        n = haeun_ipc_peek(ctx->ipc, (int)a0, pid, &peek);
        if (n < 0) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (n == 0) {
            haeun_rf_write(vm, 1u, 0u);
            return true;
        }
        haeun_rf_write(vm, 1u, (uint32_t)peek.len);
        haeun_rf_write(vm, 2u, (uint32_t)peek.sender_pid);
        haeun_rf_write(vm, 3u, (uint32_t)peek.type);
        haeun_rf_write(vm, 4u, (uint32_t)peek.seq);
        return true;
    }
    case HAEUN_SYS_WIN_QUERY: {
        haeun_gfx_window *w;
        uint32_t flags = 0u;
        int wid = (int)a0;
        if (ctx->wm == NULL) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (deny_win(ctx->wm, pid, wid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (wid < 0 || wid >= HAEUN_GFX_WM_MAX_WINDOWS ||
            !ctx->wm->windows[wid].used) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        w = &ctx->wm->windows[wid];
        if (ctx->wm->focus_id == wid)
            flags |= 1u;
        if (w->minimized)
            flags |= 2u;
        if (w->maximized)
            flags |= 4u;
        if (w->visible)
            flags |= 8u;
        haeun_rf_write(vm, 1u, 0u);
        haeun_rf_write(vm, 2u, (uint32_t)w->x);
        haeun_rf_write(vm, 3u, (uint32_t)w->y);
        haeun_rf_write(vm, 4u, (w->w & 0xffffu) | ((w->h & 0xffffu) << 16));
        haeun_rf_write(vm, 5u, flags);
        return true;
    }
    default:
        return false;
    }
}
