#include "haeun/ecall_mux.h"
#include "haeun/exception_handler.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/mmu.h"
#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/proc_hfs.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    wr32(ram + 0xA400u, curr);
    wr32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

static int test_cause_class(void) {
    CHECK(haeun_trap_cause_is_user_fatal(0u));
    CHECK(haeun_trap_cause_is_user_fatal(1u));
    CHECK(haeun_trap_cause_is_user_fatal(2u));
    CHECK(haeun_trap_cause_is_user_fatal(4u));
    CHECK(haeun_trap_cause_is_user_fatal(5u));
    CHECK(!haeun_trap_cause_is_user_fatal(3u));
    CHECK(!haeun_trap_cause_is_user_fatal(16u));
    return 0;
}

static int g_faults;

static void count_fault(haeun_vm *vm, void *ctx, uint32_t cause, uint32_t tval,
                        uint32_t epc) {
    (void)vm;
    (void)ctx;
    (void)tval;
    (void)epc;
    if (haeun_trap_cause_is_user_fatal(cause))
        g_faults++;
}

static int test_trap_enter_user_vs_kernel(void) {
    haeun_vm vm;
    uint8_t ram[4096];
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    vm.csr_trapvec = 0x100u;
    g_faults = 0;
    haeun_vm_set_fault_io(&vm, count_fault, NULL);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0x2000u;
    CHECK(haeun_exception_raise(&vm, HAEUN_VM_MEMORY_FAULT, 5u, 0xABCDu) ==
          HAEUN_VM_OK);
    CHECK(g_faults == 1);
    CHECK(vm.csr_cause == 5u);
    CHECK(vm.csr_tval == 0xABCDu);
    CHECK(vm.privilege == HAEUN_PRIV_SUPERVISOR);
    g_faults = 0;
    vm.privilege = HAEUN_PRIV_SUPERVISOR;
    vm.pc = 0x3000u;
    CHECK(haeun_exception_raise(&vm, HAEUN_VM_MEMORY_FAULT, 5u, 0u) ==
          HAEUN_VM_OK);
    CHECK(g_faults == 0); /* kernel path does not call fault_io */
    return 0;
}

static int test_fault_cleanup(void) {
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx gctx;
    haeun_ecall_mux_ctx mux;
    haeun_proc_ft ft;
    haeun_net_socks ss;
    haeun_net_if nif;
    haeun_sock_ecall_ctx sctx;
    uint8_t ram[128 * 1024], mac[6] = {2, 0, 0, 0, 0, 1};
    int wid = -1, sfd = -1;
    uint32_t fd = 0u;
    memset(ram, 0, sizeof(ram));
    memset(&mux, 0, sizeof(mux));
    memset(&sctx, 0, sizeof(sctx));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    set_pcb_pid(ram, 0u, 7u);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x4000u, &wid) == HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, wid, 7);
    haeun_proc_ft_init(&ft, NULL);
    CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, fd, 7) == HAEUN_HFS_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_set_owner(&ss, sfd, 7) == HAEUN_NET_OK);
    gctx.gfx = &gfx;
    gctx.wm = &wm;
    sctx.ss = &ss;
    mux.gfx_ctx = &gctx;
    mux.proc_ft = &ft;
    mux.sock_ctx = &sctx;
    haeun_vm_set_fault_io(&vm, haeun_ecall_mux_on_fault, &mux);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0x5000u;
    vm.csr_trapvec = 0x100u;
    CHECK(haeun_exception_raise(&vm, HAEUN_VM_MEMORY_FAULT, 5u, 0x2000u) ==
          HAEUN_VM_OK);
    CHECK(!wm.windows[wid].used);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_FREE);
    CHECK(!ss.socks[sfd].used);
    /* idempotent second fault cleanup */
    set_pcb_pid(ram, 0u, 7u);
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(haeun_exception_raise(&vm, HAEUN_VM_ILLEGAL_INSTRUCTION, 0u, 0u) ==
          HAEUN_VM_OK);
    return 0;
}

static int test_ro_code_fault_translate(void) {
    haeun_vm vm;
    uint8_t ram[64 * 1024];
    uint32_t pa = 0u, cause = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    wr32(ram + 0x1000u, 0x2000u | 1u);
    wr32(ram + 0x2000u + ((0x108000u >> 12) * 4u), 0x3000u | 0x1bu); /* no W */
    vm.csr_ptbr = 0x1001u;
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(!haeun_mmu_translate(&vm, 0x00108000u, HAEUN_MMU_ACCESS_W, &pa,
                               &cause));
    CHECK(cause == HAEUN_CAUSE_PAGE_FAULT);
    CHECK(haeun_mmu_translate(&vm, 0x00108000u, HAEUN_MMU_ACCESS_X, &pa,
                              &cause));
    return 0;
}

int main(void) {
    if (test_cause_class() != 0)
        return 1;
    if (test_trap_enter_user_vs_kernel() != 0)
        return 1;
    if (test_fault_cleanup() != 0)
        return 1;
    if (test_ro_code_fault_translate() != 0)
        return 1;
    puts("m17_fault_tests ok");
    return 0;
}
