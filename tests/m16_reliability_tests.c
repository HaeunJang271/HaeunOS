#include "haeun/app_env.h"
#include "haeun/gfx.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
#include "haeun/mmu.h"
#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/proc_hfs.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
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

static void map_slot_l0(uint8_t *ram, uint32_t l1, uint32_t l0, uint32_t spa) {
    wr32(ram + l1, l0 | 1u);
    wr32(ram + l0 + ((spa >> 12) * 4u), spa | 0x17u);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    wr32(ram + 0xA400u, curr);
    wr32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

static int test_fault_inputs(void) {
    haeun_app_env env;
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx gctx;
    haeun_proc_ft ft;
    haeun_net_socks ss;
    haeun_net_if nif;
    uint8_t ram[128 * 1024];
    uint8_t bad[16], img[256];
    size_t len = 0u;
    uint8_t mac[6] = {2, 0, 0, 0, 0, 1};
    int sfd = -1;
    uint32_t fd = 0u, pa = 0u, cause = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "x", bad, sizeof(bad)) == HAEUN_APP_INVALID);
    make_tiny_hxf(img, &len, 0x11);
    wr32(img + 16, 0x00109000u); /* wrong entry */
    CHECK(haeun_app_register(&env, "badent", img, len) == HAEUN_APP_INVALID);
    make_tiny_hxf(img, &len, 0x22);
    wr32(img + 80, 0x5000u); /* filesz > 4KiB cap */
    wr32(img + 84, 0x5000u);
    CHECK(haeun_app_register(&env, "big", img, len) == HAEUN_APP_INVALID);
    vm.csr_ptbr = 1u;
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(!haeun_mmu_translate(&vm, 0x00108000u, HAEUN_MMU_ACCESS_R, &pa,
                               &cause));
    CHECK(haeun_sec_validate_syscall(HAEUN_PRIV_USER, 999u) != HAEUN_SEC_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    gctx.gfx = &gfx;
    gctx.wm = &wm;
    set_pcb_pid(ram, 0u, 2u);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_WIN_FILL);
    haeun_rf_write(&vm, 2u, 99u);
    CHECK(haeun_gfx_ecall_io(&vm, &gctx));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    haeun_proc_ft_init(&ft, NULL);
    CHECK(haeun_proc_read(&ft, 3u, ram, 1u, &fd) != HAEUN_HFS_OK); /* bad fd */
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_sock_sendto(&ss, 0, mac, 1u, 1u, (const uint8_t *)"x", 1u) !=
          HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_close(&ss, sfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_close(&ss, sfd) != HAEUN_NET_OK);
    return 0;
}

static int test_spawn_exit_loop(void) {
    haeun_app_env env;
    haeun_vm vm;
    uint8_t *ram;
    uint8_t img[256];
    size_t len = 0u;
    uint32_t pid = 0u;
    int i;
    ram = (uint8_t *)calloc(2u * 1024u * 1024u, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_vm_init_version(&vm, ram, 2u * 1024u * 1024u, 0u, 4u) ==
          HAEUN_VM_OK);
    map_slot_l0(ram, 0x6000u, 0x7000u, 0x00114000u);
    map_slot_l0(ram, 0x8000u, 0x9000u, 0x00115000u);
    wr32(ram + 0xA404u, 2u);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len, 0xAAu);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    for (i = 0; i < 32; ++i) {
        CHECK(haeun_app_spawn(&env, 0, &pid) == HAEUN_APP_OK);
        CHECK(haeun_app_reap(&env, pid) == HAEUN_APP_OK);
        wr32(ram + 0xA100u, 0u); /* FREE slot1 */
        wr32(ram + 0xA200u, 0u); /* FREE slot2 */
    }
    free(ram);
    return 0;
}

static int test_resource_exhaustion(void) {
    haeun_app_env env;
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_proc_ft ft;
    haeun_net_socks ss;
    haeun_net_if nif;
    uint8_t *ram;
    uint8_t img[256], mac[6] = {2, 0, 0, 0, 0, 1};
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u, p3 = 0u, fd = 0u;
    int ids[HAEUN_GFX_WM_MAX_WINDOWS + 1];
    int socks[HAEUN_NET_MAX_SOCKS + 1];
    int i, extra = -1;
    ram = (uint8_t *)calloc(2u * 1024u * 1024u, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_vm_init_version(&vm, ram, 2u * 1024u * 1024u, 0u, 4u) ==
          HAEUN_VM_OK);
    map_slot_l0(ram, 0x6000u, 0x7000u, 0x00114000u);
    map_slot_l0(ram, 0x8000u, 0x9000u, 0x00115000u);
    wr32(ram + 0xA404u, 2u);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len, 0x33);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p2) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_NOSPC);
    CHECK(haeun_app_reap(&env, p1) == HAEUN_APP_OK);
    wr32(ram + 0xA100u, 0u);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u,
                                  0x4000u + (uint32_t)i * 0x100u, &ids[i]) ==
              HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x4800u, &extra) !=
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_destroy(&wm, ids[0]) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x4000u, &extra) ==
          HAEUN_GFX_OK);
    haeun_proc_ft_init(&ft, NULL);
    for (i = 0; i < (int)HAEUN_PROC_NFD; ++i)
        CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_NOSPC);
    CHECK(haeun_proc_close(&ft, 0u) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    for (i = 0; i < HAEUN_NET_MAX_SOCKS; ++i)
        CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &socks[i]) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &extra) == HAEUN_NET_NOSPACE);
    CHECK(haeun_sock_close(&ss, socks[0]) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &extra) == HAEUN_NET_OK);
    free(ram);
    return 0;
}

static int test_gui_net_lifecycle(void) {
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_net_socks ss;
    haeun_net_if nif;
    uint8_t ram[128 * 1024], mac[6] = {2, 0, 0, 0, 0, 1};
    int id = -1, sfd = -1, i;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    for (i = 0; i < 16; ++i) {
        CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x4000u, &id) ==
              HAEUN_GFX_OK);
        (void)haeun_gfx_wm_set_owner(&wm, id, 5);
        CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
        (void)haeun_sock_set_owner(&ss, sfd, 5);
        CHECK(haeun_socks_close_by_owner(&ss, 5) == HAEUN_NET_OK);
        CHECK(haeun_app_destroy_windows(&wm, 5u) == HAEUN_APP_OK);
        CHECK(!wm.windows[id].used);
        CHECK(!ss.socks[sfd].used);
    }
    CHECK(haeun_gfx_put_pixel(&gfx, 999u, 999u, 1u) != HAEUN_GFX_OK);
    return 0;
}

int main(void) {
    if (test_fault_inputs() != 0)
        return 1;
    if (test_spawn_exit_loop() != 0)
        return 1;
    if (test_resource_exhaustion() != 0)
        return 1;
    if (test_gui_net_lifecycle() != 0)
        return 1;
    puts("m16_reliability_tests ok");
    return 0;
}
