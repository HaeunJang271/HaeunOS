/**
 * M20.8 security extras: sock/IPC/FD isolation, stale IDs, 100× reclaim loops.
 */
#include "m20_integration_harness.h"

#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/hfs.h"
#include "haeun/ipc.h"
#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/proc_hfs.h"
#include "haeun/vm.h"

#include <string.h>

/** Foreign sock/IPC/FD denied; queue overflow; stale window; victim intact. */
int m20i_test_sock_ipc_fd_security(void) {
    static uint8_t ram[256 * 1024];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    haeun_ipc_hub ipc;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint8_t himg[16u * 512u];
    uint8_t msg[4] = {1, 2, 3, 4};
    uint8_t out[8];
    uint8_t mac[6] = {2, 0, 0, 0, 0, 1};
    int fd = -1, epid = -1, wid = -1, i, sender = 0;
    uint32_t pfd = 0u, n = 0u;
    uint8_t b = 0u;

    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &fd) == HAEUN_NET_OK);
    CHECK(haeun_sock_set_owner(&ss, fd, 7) == HAEUN_NET_OK);
    CHECK(haeun_sock_owned_by(&ss, fd, 99) == 0);
    CHECK(haeun_sock_owned_by(&ss, fd, 7) == 1);
    CHECK(haeun_sock_close(&ss, 99) == HAEUN_NET_BAD_ARGUMENT);
    /* Wrong-owner reap must not touch victim socket. */
    CHECK(haeun_socks_close_by_owner(&ss, 99) == HAEUN_NET_OK);
    CHECK(haeun_sock_owned_by(&ss, fd, 7) == 1);
    CHECK(haeun_socks_close_by_owner(&ss, 7) == HAEUN_NET_OK);
    CHECK(haeun_sock_owned_by(&ss, fd, 7) == 0);

    haeun_ipc_hub_init(&ipc);
    epid = haeun_ipc_create(&ipc, 5);
    CHECK(epid >= 0);
    for (i = 0; i < HAEUN_IPC_QUEUE; ++i)
        CHECK(haeun_ipc_send(&ipc, epid, 3, msg, 4u) == 0);
    CHECK(haeun_ipc_send(&ipc, epid, 3, msg, 4u) == -2);
    CHECK(haeun_ipc_send(&ipc, epid, 3, msg, 0u) == -3);
    CHECK(haeun_ipc_send(&ipc, epid, 3, msg, 64u) == -3);
    CHECK(haeun_ipc_recv(&ipc, epid, 99, out, 8u, &sender) < 0);
    CHECK(haeun_ipc_recv(&ipc, epid, 5, out, 8u, &sender) == 4);
    CHECK(sender == 3);
    CHECK(haeun_ipc_close(&ipc, epid, 99) < 0);
    CHECK(haeun_ipc_close(&ipc, epid, 5) == 0);
    /* Endpoint reuse after close. */
    epid = haeun_ipc_create(&ipc, 5);
    CHECK(epid >= 0);
    CHECK(haeun_ipc_send(&ipc, epid, 3, msg, 2u) == 0);

    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 16u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x20000u, &wid) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, wid, 11) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_owned_by(&wm, wid, 22) == 0);
    CHECK(haeun_gfx_wm_destroy(&wm, wid) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_owned_by(&wm, wid, 11) == 0);
    CHECK(haeun_gfx_wm_fill(&wm, wid, 0xffu) != HAEUN_GFX_OK);

    CHECK(haeun_hfs_mkfs(himg, sizeof(himg), 16u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, himg, sizeof(himg)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "own", 1u, &pfd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, pfd, 8) == HAEUN_HFS_OK);
    CHECK(haeun_proc_write(&ft, pfd, (const uint8_t *)"Q", 1u, &n) ==
          HAEUN_HFS_OK);
    /* Host API has no owner gate; ECALL does — verify owner reap leaves data. */
    CHECK(haeun_proc_fds_close_by_owner(&ft, 9) == HAEUN_HFS_OK);
    CHECK(haeun_proc_seek(&ft, pfd, 0u) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, pfd, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(b == 'Q');
    CHECK(haeun_proc_fds_close_by_owner(&ft, 8) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, pfd, &b, 1u, &n) == HAEUN_HFS_BAD_ARGUMENT);
    /* Bad path / bad fd. */
    CHECK(haeun_proc_open_path(&ft, "", 1u, &pfd) == HAEUN_HFS_BAD_ARGUMENT);
    CHECK(haeun_proc_close(&ft, 99u) == HAEUN_HFS_BAD_ARGUMENT);
    return 0;
}

/** 100× window / IPC / socket create-destroy reclaim. */
int m20i_test_repeat_win_ipc_sock(void) {
    static uint8_t ram[512 * 1024];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_ipc_hub ipc;
    haeun_net_if nif;
    haeun_net_socks ss;
    uint8_t mac[6] = {2, 0, 0, 0, 0, 1};
    int i, wid = -1, epid = -1, sfd = -1;

    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 16u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_ipc_hub_init(&ipc);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, 0x2000u, 0x3000u, 512u) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);

    for (i = 0; i < 100; ++i) {
        CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x20000u, &wid) ==
              HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_set_owner(&wm, wid, 2) == HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_destroy(&wm, wid) == HAEUN_GFX_OK);

        epid = haeun_ipc_create(&ipc, 2);
        CHECK(epid >= 0);
        CHECK(haeun_ipc_close(&ipc, epid, 2) == 0);

        CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
        CHECK(haeun_sock_set_owner(&ss, sfd, 2) == HAEUN_NET_OK);
        CHECK(haeun_socks_close_by_owner(&ss, 2) == HAEUN_NET_OK);
    }
    /* Table still usable after 100 cycles. */
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x20000u, &wid) ==
          HAEUN_GFX_OK);
    epid = haeun_ipc_create(&ipc, 2);
    CHECK(epid >= 0);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &sfd) == HAEUN_NET_OK);
    return 0;
}
