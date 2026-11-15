#include "haeun/guest_pcb.h"
#include "haeun/mmu.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stdio.h>
#include <string.h>

static void sock_log(const char *detail) {
    fprintf(stderr, "[haeun-sock-ecall] failure: %s\n",
            detail != NULL ? detail : "");
}

static bool guest_load_u8(haeun_vm *vm, uint32_t va, uint8_t *out) {
    uint32_t pa = 0u;
    uint32_t cause = 0u;
    if (vm == NULL || out == NULL) return false;
    if ((vm->csr_ptbr & 1u) == 0u) {
        if ((uint64_t)va >= (uint64_t)vm->ram_size) return false;
        *out = vm->ram[va];
        return true;
    }
    if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_R, &pa, &cause))
        return false;
    if ((uint64_t)pa >= (uint64_t)vm->ram_size) return false;
    *out = vm->ram[pa];
    return true;
}

static bool guest_store_u8(haeun_vm *vm, uint32_t va, uint8_t b) {
    uint32_t pa = 0u;
    uint32_t cause = 0u;
    if (vm == NULL) return false;
    if ((vm->csr_ptbr & 1u) == 0u) {
        if ((uint64_t)va >= (uint64_t)vm->ram_size) return false;
        vm->ram[va] = b;
        return true;
    }
    if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_W, &pa, &cause))
        return false;
    if ((uint64_t)pa >= (uint64_t)vm->ram_size) return false;
    vm->ram[pa] = b;
    return true;
}

static bool copy_from_guest(haeun_vm *vm, uint32_t va, uint8_t *out,
                            uint32_t len) {
    uint32_t i;
    for (i = 0u; i < len; ++i) {
        if (!guest_load_u8(vm, va + i, &out[i])) return false;
    }
    return true;
}

static bool copy_to_guest(haeun_vm *vm, uint32_t va, const uint8_t *in,
                          uint32_t len) {
    uint32_t i;
    for (i = 0u; i < len; ++i) {
        if (!guest_store_u8(vm, va + i, in[i])) return false;
    }
    return true;
}

static uint32_t net_to_ret(haeun_net_result r, uint32_t ok_value) {
    if (r == HAEUN_NET_OK) return ok_value;
    return 0xffffffffu;
}

static int current_pid(haeun_vm *vm) {
    return (int)haeun_guest_pcb_current_pid(vm);
}

/**
 * Deny when fd is missing or not owned by the caller.
 * pid==0 (no PCB) may only use unowned sockets (legacy unit tests).
 */
static int deny_sock(haeun_net_socks *ss, int fd, int pid) {
    if (!haeun_sock_owned_by(ss, fd, pid)) {
        sock_log("socket ownership denied");
        return 1;
    }
    return 0;
}

/**
 * Completes socket syscalls on the host when a sock ECALL context is installed.
 */
bool haeun_sock_ecall_io(haeun_vm *vm, void *context) {
    haeun_sock_ecall_ctx *ctx = (haeun_sock_ecall_ctx *)context;
    uint32_t number;
    uint32_t a0, a1, a2, a3, a4, a5;
    int fd = 0;
    int out_fd = 0;
    int pid;
    uint32_t got = 0u;
    uint32_t src_ip = 0u;
    uint16_t src_port = 0u;
    uint8_t buf[HAEUN_NET_UDP_MAX];
    haeun_net_result r;
    haeun_sec_syscall_req sreq;
    if (vm == NULL || ctx == NULL || ctx->ss == NULL) return false;
    if (vm->privilege != HAEUN_PRIV_USER) return false;
    number = haeun_rf_read(vm, 1u);
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    a3 = haeun_rf_read(vm, 5u);
    a4 = haeun_rf_read(vm, 6u);
    a5 = haeun_rf_read(vm, 7u);
    pid = current_pid(vm);
    switch (number) {
    case HAEUN_SYS_SOCKET:
        r = haeun_socket(ctx->ss, (int)a0, &fd);
        if (r == HAEUN_NET_OK && pid != 0)
            (void)haeun_sock_set_owner(ctx->ss, fd, pid);
        haeun_rf_write(vm, 1u, net_to_ret(r, (uint32_t)fd));
        return true;
    case HAEUN_SYS_BIND:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_bind(ctx->ss, (int)a0, (uint16_t)a1);
        haeun_rf_write(vm, 1u, net_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_SENDTO:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_lo = 0u;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (a4 == 0u || a4 > HAEUN_NET_UDP_MAX) {
            sock_log("sendto bad len");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (haeun_sec_check_user_va(&sreq, a3, HAEUN_MMU_ACCESS_R) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (!copy_from_guest(vm, a3, buf, a4)) {
            sock_log("sendto copy");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_sendto(ctx->ss, (int)a0, ctx->next_hop_mac, a1,
                              (uint16_t)a2, buf, a4);
        haeun_rf_write(vm, 1u, net_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_RECVFROM:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        memset(&sreq, 0, sizeof(sreq));
        sreq.privilege = HAEUN_PRIV_USER;
        sreq.vm = vm;
        sreq.user_lo = 0u;
        sreq.user_hi = (uint32_t)vm->ram_size;
        if (a2 == 0u || a2 > HAEUN_NET_UDP_MAX) {
            sock_log("recvfrom bad cap");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (haeun_sec_check_user_va(&sreq, a1, HAEUN_MMU_ACCESS_W) !=
            HAEUN_SEC_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_recvfrom(ctx->ss, (int)a0, &src_ip, &src_port, buf, a2,
                                &got);
        if (r != HAEUN_NET_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (!copy_to_guest(vm, a1, buf, got)) {
            sock_log("recvfrom copy");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (a3 != 0u) {
            uint8_t meta[6];
            meta[0] = (uint8_t)(src_ip >> 24);
            meta[1] = (uint8_t)(src_ip >> 16);
            meta[2] = (uint8_t)(src_ip >> 8);
            meta[3] = (uint8_t)src_ip;
            meta[4] = (uint8_t)(src_port >> 8);
            meta[5] = (uint8_t)src_port;
            if (!copy_to_guest(vm, a3, meta, 6u)) {
                sock_log("recvfrom meta copy");
                haeun_rf_write(vm, 1u, 0xffffffffu);
                return true;
            }
        }
        (void)a5;
        haeun_rf_write(vm, 1u, got);
        return true;
    case HAEUN_SYS_LISTEN:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_listen(ctx->ss, (int)a0);
        haeun_rf_write(vm, 1u, net_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_ACCEPT:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_accept(ctx->ss, (int)a0, &out_fd, &src_ip, &src_port);
        if (r == HAEUN_NET_OK && pid != 0)
            (void)haeun_sock_set_owner(ctx->ss, out_fd, pid);
        haeun_rf_write(vm, 1u, net_to_ret(r, (uint32_t)out_fd));
        return true;
    case HAEUN_SYS_SO_CLOSE:
        if (deny_sock(ctx->ss, (int)a0, pid)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_sock_close(ctx->ss, (int)a0);
        haeun_rf_write(vm, 1u, net_to_ret(r, 0u));
        return true;
    default:
        return false;
    }
}
