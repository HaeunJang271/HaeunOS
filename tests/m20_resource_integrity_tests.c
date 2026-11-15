/**
 * M20.0.2: WIN_CREATE PA safety, socket ownership, PCB/AS capacity invariants.
 */
#include "haeun/app_env.h"
#include "haeun/gfx.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
#include "haeun/guest_pcb.h"
#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define ENC(op, rd, ra, rb, imm)                                               \
    (((uint32_t)(op) << 26) | ((uint32_t)(rd) << 22) | ((uint32_t)(ra) << 18) | \
     ((uint32_t)(rb) << 14) | ((uint32_t)(imm) & 0x3ffffu))

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    put_le32(ram + 0xA400u, curr);
    put_le32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

/** Rejects unaligned, OOB, FB overlap, and foreign pool PA for process apps. */
static int test_win_create_pa_rejects(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx ctx;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x20001u, &id) ==
          HAEUN_GFX_BAD_ARGUMENT);
    CHECK(!haeun_gfx_wm_backing_allowed(&wm, &gfx, 0x10000u, 8u, 8u));
    CHECK(haeun_gfx_wm_in_user_pool(0x00020000u, 24u, 12u));
    CHECK(!haeun_gfx_wm_in_user_pool(0x0000A000u, 1u, 1u));
    CHECK(!haeun_gfx_wm_in_user_pool(0x00108000u, 1u, 1u));
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    haeun_vm_set_ecall_io(&vm, haeun_gfx_ecall_io, &ctx);
    set_pcb_pid(ram, 1u, 5u);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 0u;
    vm.r[3] = 0u;
    vm.r[4] = 8u;
    vm.r[5] = 8u;
    vm.r[6] = 0x0000A000u; /* PCB bank — must reject for pid!=0 */
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[6] = 0x00020000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] < (uint32_t)HAEUN_GFX_WM_MAX_WINDOWS);
    id = (int)vm.r[1];
    CHECK(wm.windows[id].owner_pid == 5);
    return 0;
}

/** Foreign PID cannot FILL/DESTROY; host-unowned chrome stays protected. */
static int test_window_owner_deny(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx ctx;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x00020000u, &id) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, id, 3) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_owned_by(&wm, id, 3) == 1);
    CHECK(haeun_gfx_wm_owned_by(&wm, id, 4) == 0);
    CHECK(haeun_gfx_wm_owned_by(&wm, id, 0) == 0);
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    haeun_vm_set_ecall_io(&vm, haeun_gfx_ecall_io, &ctx);
    set_pcb_pid(ram, 0u, 4u);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = (uint32_t)id;
    vm.r[3] = 0x00ff0000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    return 0;
}

/** Cross-pid BIND/SENDTO/CLOSE denied; ACCEPT inherits owner; bad fd rejected. */
static int test_socket_ownership(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    haeun_sock_ecall_ctx ctx;
    uint8_t mac[6] = {2, 0, 0, 0, 0, 1};
    int fd = -1;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    memset(&ctx, 0, sizeof(ctx));
    ctx.ss = &ss;
    haeun_vm_set_ecall_io(&vm, haeun_sock_ecall_io, &ctx);
    set_pcb_pid(ram, 0u, 2u);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_SOCKET;
    vm.r[2] = (uint32_t)HAEUN_SOCK_DGRAM;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    fd = (int)vm.r[1];
    CHECK(fd >= 0 && fd < HAEUN_NET_MAX_SOCKS);
    CHECK(haeun_sock_owned_by(&ss, fd, 2) == 1);
    CHECK(haeun_sock_owned_by(&ss, fd, 9) == 0);
    set_pcb_pid(ram, 0u, 9u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_BIND;
    vm.r[2] = (uint32_t)fd;
    vm.r[3] = 7u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_SO_CLOSE;
    vm.r[2] = (uint32_t)fd;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_SO_CLOSE;
    vm.r[2] = 99u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    CHECK(haeun_socks_close_by_owner(&ss, 2) == HAEUN_NET_OK);
    CHECK(ss.socks[fd].used == 0);
    return 0;
}

/** Capacity invariants: register max, AS slots, PCB table size. */
static int test_capacity_invariants(void) {
    CHECK((int)HAEUN_APP_MAX == 5);
    CHECK((int)HAEUN_APP_SLOTS == 3);
    CHECK((int)HAEUN_GUEST_PCB_AS_SLOTS == (int)HAEUN_APP_SLOTS);
    CHECK((int)HAEUN_GUEST_PCB_NPROC >= (int)HAEUN_GUEST_PCB_AS_SLOTS);
    CHECK(HAEUN_WIN_POOL_LO == 0x00018000u);
    CHECK(HAEUN_WIN_POOL_HI == 0x00100000u);
    return 0;
}

/** Overlapping window backing rejected; destroy reclaims for reuse. */
static int test_backing_overlap_and_reuse(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int a = -1, b = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 16u, 0x00020000u, &a) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x00020000u, &b) ==
          HAEUN_GFX_BAD_ARGUMENT);
    CHECK(haeun_gfx_wm_destroy(&wm, a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x00020000u, &b) ==
          HAEUN_GFX_OK);
    return 0;
}

int main(void) {
    CHECK(test_capacity_invariants() == 0);
    CHECK(test_win_create_pa_rejects() == 0);
    CHECK(test_window_owner_deny() == 0);
    CHECK(test_socket_ownership() == 0);
    CHECK(test_backing_overlap_and_reuse() == 0);
    (void)printf("m20_resource_integrity_tests ok\n");
    return 0;
}
