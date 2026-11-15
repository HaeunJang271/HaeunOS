#include "haeun/app_env.h"
#include "haeun/board_mmio.h"
#include "haeun/gfx.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
#include "haeun/mmu.h"
#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/proc_hfs.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
#include "haeun/tlb.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
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

static void make_tiny_hxf(uint8_t *out, size_t *out_len, uint8_t tag) {
    uint8_t code[8] = {0, 0, 0, 0, 0x00, 0x00, 0x00, 0x04};
    size_t total = 96u + sizeof(code);
    code[0] = tag;
    memset(out, 0, total);
    out[0] = 'H';
    out[1] = 'X';
    out[2] = 'F';
    out[3] = '1';
    out[4] = 64;
    out[6] = 1;
    out[8] = 4;
    wr32(out + 16, 0x00108000u);
    out[20] = 64;
    out[24] = 1;
    out[26] = 32;
    wr32(out + 28, (uint32_t)total);
    wr32(out + 64, 1u);
    wr32(out + 68, 7u);
    wr32(out + 72, 96u);
    wr32(out + 76, 0x00108000u);
    wr32(out + 80, (uint32_t)sizeof(code));
    wr32(out + 84, (uint32_t)sizeof(code));
    wr32(out + 88, 4u);
    memcpy(out + 96, code, sizeof(code));
    *out_len = total;
}

static void map_slot_l0(uint8_t *ram, uint32_t l1, uint32_t l0, uint32_t stack_pa) {
    wr32(ram + l1, l0 | 1u);
    wr32(ram + l0 + 0x420u, 0x00108000u | 0x1bu); /* placeholder until spawn */
    wr32(ram + l0 + ((stack_pa >> 12) * 4u), stack_pa | 0x17u);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    wr32(ram + 0xA400u, curr);
    wr32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

static int test_invalid_hxf(void) {
    haeun_app_env env;
    haeun_vm vm;
    uint8_t ram[8 * 1024];
    uint8_t bad[8] = {0};
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "x", bad, sizeof(bad)) == HAEUN_APP_INVALID);
    return 0;
}

static int test_pte_tlb_reuse(void) {
    haeun_app_env env;
    haeun_vm vm;
    uint8_t *ram;
    uint8_t img_a[256], img_b[256];
    size_t la = 0u, lb = 0u;
    uint32_t pid1 = 0u, pid2 = 0u;
    uint32_t pte1, pte2;
    uint32_t pa1, pa2;
    uint32_t cause = 0u, out_pa = 0u;
    ram = (uint8_t *)calloc(2u * 1024u * 1024u, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_vm_init_version(&vm, ram, 2u * 1024u * 1024u, 0u, 4u) ==
          HAEUN_VM_OK);
    map_slot_l0(ram, 0x6000u, 0x7000u, 0x00114000u);
    map_slot_l0(ram, 0x8000u, 0x9000u, 0x00115000u);
    wr32(ram + 0xA404u, 2u);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img_a, &la, 0xAAu);
    make_tiny_hxf(img_b, &lb, 0xBBu);
    CHECK(haeun_app_register(&env, "a", img_a, la) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "b", img_b, lb) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &pid1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 1, &pid2) == HAEUN_APP_OK);
    pa1 = haeun_app_slot_code_pa(1);
    pa2 = haeun_app_slot_code_pa(2);
    pte1 = haeun_app_slot_user_pte(&env, 1);
    pte2 = haeun_app_slot_user_pte(&env, 2);
    CHECK((pte1 & ~0xfffu) == pa1);
    CHECK((pte2 & ~0xfffu) == pa2);
    CHECK((pte1 & 0x1bu) == 0x1bu); /* V|R|X|U, no W */
    CHECK((pte1 & 4u) == 0u);
    CHECK(ram[pa1] == 0xAAu);
    CHECK(ram[pa2] == 0xBBu);
    /* AS1 cannot translate AS2 stack VA */
    vm.csr_ptbr = 0x00006001u;
    vm.privilege = HAEUN_PRIV_USER;
    haeun_tlb_flush(&vm);
    CHECK(!haeun_mmu_translate(&vm, 0x00115000u, HAEUN_MMU_ACCESS_R, &out_pa,
                               &cause));
    /* AS1 can translate own stack */
    CHECK(haeun_mmu_translate(&vm, 0x00114000u, HAEUN_MMU_ACCESS_R, &out_pa,
                              &cause));
    CHECK(out_pa == 0x00114000u);
    /* own code VA → slot1 PA */
    CHECK(haeun_mmu_translate(&vm, 0x00108000u, HAEUN_MMU_ACCESS_X, &out_pa,
                              &cause));
    CHECK(out_pa == pa1);
    /* reap scrub: wipe code + invalidate PTE */
    CHECK(haeun_app_reap(&env, pid1) == HAEUN_APP_OK);
    CHECK(ram[pa1] == 0u);
    CHECK(haeun_app_slot_user_pte(&env, 1) == 0u);
    /* reuse slot1 with other image */
    wr32(ram + 0xA100u, 0u); /* PCB slot1 FREE */
    CHECK(haeun_app_spawn(&env, 1, &pid1) == HAEUN_APP_OK);
    CHECK(ram[haeun_app_slot_code_pa(1)] == 0xBBu);
    CHECK((haeun_app_slot_user_pte(&env, 1) & ~0xfffu) ==
          haeun_app_slot_code_pa(1));
    free(ram);
    return 0;
}

static int test_window_ownership(void) {
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx ctx;
    uint8_t ram[256 * 1024];
    int id_a = -1, id_b = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    set_pcb_pid(ram, 0u, 2u);
    vm.privilege = HAEUN_PRIV_USER;
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_CREATE);
    haeun_rf_write(&vm, 2u, 0u);
    haeun_rf_write(&vm, 3u, 0u);
    haeun_rf_write(&vm, 4u, 8u);
    haeun_rf_write(&vm, 5u, 8u);
    haeun_rf_write(&vm, 6u, 0x00020000u);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    id_a = (int)haeun_rf_read(&vm, 1u);
    CHECK(id_a >= 0);
    CHECK(wm.windows[id_a].owner_pid == 2);
    set_pcb_pid(ram, 1u, 3u);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_CREATE);
    haeun_rf_write(&vm, 6u, 0x00022000u);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    id_b = (int)haeun_rf_read(&vm, 1u);
    CHECK(id_b >= 0 && id_b != id_a);
    /* pid 2 cannot FILL pid 3 window */
    set_pcb_pid(ram, 0u, 2u);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_FILL);
    haeun_rf_write(&vm, 2u, (uint32_t)id_b);
    haeun_rf_write(&vm, 3u, 0xffu);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_DESTROY);
    haeun_rf_write(&vm, 2u, (uint32_t)id_b);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_RAISE);
    haeun_rf_write(&vm, 2u, (uint32_t)id_b);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    /* own window ok */
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_FILL);
    haeun_rf_write(&vm, 2u, (uint32_t)id_a);
    CHECK(haeun_gfx_ecall_io(&vm, &ctx));
    CHECK(haeun_rf_read(&vm, 1u) == 0u);
    /* EXIT cleanup + id reuse clears owner */
    CHECK(haeun_app_destroy_windows(&wm, 3u) == HAEUN_APP_OK);
    CHECK(!wm.windows[id_b].used);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x00022000u, &id_b) ==
          HAEUN_GFX_OK);
    CHECK(wm.windows[id_b].owner_pid == 0);
    return 0;
}

static int test_fd_and_socket_cleanup(void) {
    haeun_proc_ft ft;
    haeun_net_socks ss;
    haeun_net_if nif;
    haeun_vm vm;
    uint8_t ram[64 * 1024];
    uint32_t fd = 0u;
    int sfd = 0;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    haeun_proc_ft_init(&ft, NULL);
    CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, fd, 9) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fds_close_by_owner(&ft, 9) == HAEUN_HFS_OK);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_FREE);
    {
        uint8_t mac[6] = {0x02, 0, 0, 0, 0, 1};
        CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
        CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    }
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_set_owner(&ss, sfd, 9) == HAEUN_NET_OK);
    CHECK(haeun_socks_close_by_owner(&ss, 9) == HAEUN_NET_OK);
    CHECK(!ss.socks[sfd].used);
    return 0;
}

static int test_kbd_fifo_shared(void) {
    haeun_vm vm;
    uint8_t ram[4096];
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    vm.kbd_rx[0] = (uint8_t)'A';
    vm.kbd_rx[1] = (uint8_t)'B';
    vm.kbd_rx_count = 2u;
    vm.kbd_rx_head = 0u;
    /* Any consumer drains shared FIFO in order — not process-scoped. */
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE) == (uint32_t)'A');
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE) == (uint32_t)'B');
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE) == UINT32_C(0x80000000));
    return 0;
}

static int test_boundary_fault(void) {
    haeun_vm vm;
    uint8_t ram[8192];
    uint32_t pa = 0u, cause = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    vm.csr_ptbr = 1u; /* paging on, empty tables */
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(!haeun_mmu_translate(&vm, 0x00108000u, HAEUN_MMU_ACCESS_R, &pa,
                               &cause));
    return 0;
}

int main(void) {
    if (test_invalid_hxf() != 0)
        return 1;
    if (test_pte_tlb_reuse() != 0)
        return 1;
    if (test_window_ownership() != 0)
        return 1;
    if (test_fd_and_socket_cleanup() != 0)
        return 1;
    if (test_kbd_fifo_shared() != 0)
        return 1;
    if (test_boundary_fault() != 0)
        return 1;
    puts("m15_isolation_tests ok");
    return 0;
}
