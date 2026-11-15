/**
 * M20.8 core: ABI map, ownership, capacity reclaim, HFS/evt/service repeats.
 */
#include "m20_integration_harness.h"

#include "haeun/app_env.h"
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/hfs.h"
#include "haeun/ipc.h"
#include "haeun/proc_hfs.h"
#include "haeun/sec.h"
#include "haeun/svc_mgr.h"
#include "haeun/vm.h"

/** ABI map: SYS_MAX, no collision between IPC name svc and Service Manager. */
int m20i_test_abi_map(void) {
    CHECK((int)HAEUN_SYS_MAX == 50);
    CHECK((int)HAEUN_SYS_SVC_REGISTER == 33);
    CHECK((int)HAEUN_SYS_SVC_LOOKUP == 34);
    CHECK((int)HAEUN_SYS_SERVICE_COUNT == 41);
    CHECK((int)HAEUN_SYS_FS_READDIR == 47);
    CHECK((int)HAEUN_SYS_FS_SEEK == 50);
    CHECK((int)HAEUN_APP_SLOTS == 3);
    CHECK((int)HAEUN_APP_MAX == 5);
    CHECK((int)HAEUN_GFX_WM_MAX_WINDOWS == 8);
    CHECK((int)HAEUN_IPC_MAX_ENDPOINTS == 8);
    CHECK((int)HAEUN_EVT_QUEUE == 16);
    CHECK(HAEUN_PROC_NFD == 4u);
    return 0;
}

/** Foreign window / IPC / APP_CTRL / evt denied; victim FD data intact. */
int m20i_test_ownership_isolation(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_evt_hub evt;
    haeun_ipc_hub ipc;
    haeun_app_env apps;
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint8_t himg[16u * 512u];
    uint8_t appimg[256];
    size_t alen = 0u;
    int wid = -1;
    int epid;
    uint32_t pid = 0u, fd = 0u, n = 0u;
    uint8_t b = 0u;
    haeun_evt ev;
    m20i_prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&evt, &wm);
    haeun_ipc_hub_init(&ipc);
    CHECK(haeun_app_env_init(&apps, &vm, &wm) == HAEUN_APP_OK);
    apps.evt = &evt;
    m20i_make_tiny_hxf(appimg, &alen);
    CHECK(haeun_app_register(&apps, "a", appimg, alen) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &pid) == HAEUN_APP_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, 8u, 8u, 0x20000u, &wid) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, wid, (int)pid) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_owned_by(&wm, wid, 99) == 0);
    CHECK(haeun_app_ctrl(&apps, HAEUN_APP_CTRL_FOCUS, pid, 99u) ==
          HAEUN_APP_INVALID);
    CHECK(wm.windows[wid].used == 1);
    epid = haeun_ipc_create(&ipc, (int)pid);
    CHECK(epid >= 0);
    CHECK(haeun_ipc_close(&ipc, epid, 99) < 0);
    CHECK(haeun_ipc_close(&ipc, epid, (int)pid) == 0);
    CHECK(haeun_hfs_mkfs(himg, sizeof(himg), 16u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, himg, sizeof(himg)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "v", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, fd, (int)pid) == HAEUN_HFS_OK);
    CHECK(haeun_proc_write(&ft, fd, (const uint8_t *)"Z", 1u, &n) ==
          HAEUN_HFS_OK);
    CHECK(haeun_proc_seek(&ft, fd, 0u) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, fd, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(b == 'Z');
    ev.type = HAEUN_EVT_KEY_DOWN;
    ev.a = 1u;
    CHECK(haeun_evt_push(&evt, (int)pid, &ev) == 0);
    CHECK(haeun_evt_poll(&evt, 99, &ev) == 0);
    CHECK(haeun_evt_poll(&evt, (int)pid, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_KEY_DOWN);
    return 0;
}

/** Exhaust windows/IPC/FD/AS then reclaim and reuse. */
int m20i_test_resource_exhaust_recover(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_ipc_hub ipc;
    haeun_app_env apps;
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint8_t himg[16u * 512u];
    uint8_t appimg[256];
    size_t alen = 0u;
    int ids[16];
    int i, epid;
    uint32_t p1 = 0u, p2 = 0u, p3 = 0u, fd = 0u;
    m20i_prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        uint32_t pa = 0x20000u + (uint32_t)i * 0x1000u;
        CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, pa, &ids[i]) ==
              HAEUN_GFX_OK);
    }
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x30000u, &ids[0]) !=
          HAEUN_GFX_OK);
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        CHECK(haeun_gfx_wm_destroy(&wm, ids[i]) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x20000u, &ids[0]) ==
          HAEUN_GFX_OK);
    haeun_ipc_hub_init(&ipc);
    for (i = 0; i < HAEUN_IPC_MAX_ENDPOINTS; ++i)
        CHECK(haeun_ipc_create(&ipc, 3) >= 0);
    CHECK(haeun_ipc_create(&ipc, 3) < 0);
    haeun_ipc_reap(&ipc, 3);
    epid = haeun_ipc_create(&ipc, 3);
    CHECK(epid >= 0);
    CHECK(haeun_hfs_mkfs(himg, sizeof(himg), 16u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, himg, sizeof(himg)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    for (i = 0; i < (int)HAEUN_PROC_NFD; ++i) {
        char name[8];
        snprintf(name, sizeof(name), "f%d", i);
        CHECK(haeun_proc_open_path(&ft, name, 1u, &fd) == HAEUN_HFS_OK);
    }
    CHECK(haeun_proc_open_path(&ft, "x", 1u, &fd) == HAEUN_HFS_NOSPC);
    CHECK(haeun_proc_fds_close_by_owner(&ft, 0) == HAEUN_HFS_BAD_ARGUMENT);
    for (i = 0; i < (int)HAEUN_PROC_NFD; ++i)
        CHECK(haeun_proc_close(&ft, (uint32_t)i) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_path(&ft, "again", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    m20i_make_tiny_hxf(appimg, &alen);
    CHECK(haeun_app_register(&apps, "a", appimg, alen) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p2) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p3) == HAEUN_APP_NOSPC);
    m20i_wr32(ram + M20I_PCB_BASE + 1u * M20I_PCB_STRIDE, 0u);
    CHECK(haeun_app_reap(&apps, p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p3) == HAEUN_APP_OK);
    return 0;
}

/** 100× file create/write/read/unlink; remount preserves last write. */
int m20i_test_hfs_repeat_and_remount(void) {
    uint8_t img[32u * 512u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd = 0u, n = 0u, size = 0u;
    uint16_t mode = 0u;
    uint8_t b = 0u;
    int i;
    CHECK(haeun_hfs_mkfs(img, sizeof(img), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, img, sizeof(img)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    for (i = 0; i < 100; ++i) {
        char name[8];
        snprintf(name, sizeof(name), "t%d", i % 8);
        (void)haeun_proc_unlink_root(&ft, name);
        CHECK(haeun_proc_open_path(&ft, name, 1u, &fd) == HAEUN_HFS_OK);
        b = (uint8_t)('A' + (i % 26));
        CHECK(haeun_proc_write(&ft, fd, &b, 1u, &n) == HAEUN_HFS_OK);
        CHECK(haeun_proc_seek(&ft, fd, 0u) == HAEUN_HFS_OK);
        CHECK(haeun_proc_read(&ft, fd, &b, 1u, &n) == HAEUN_HFS_OK);
        CHECK(b == (uint8_t)('A' + (i % 26)));
        CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    }
    CHECK(haeun_proc_open_path(&ft, "keep", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_write(&ft, fd, (const uint8_t *)"OK", 2u, &n) ==
          HAEUN_HFS_OK);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, img, sizeof(img)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_stat_root(&ft, "keep", &size, &mode) == HAEUN_HFS_OK);
    CHECK(size == 2u);
    CHECK(haeun_proc_open_path(&ft, "keep", 0u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, fd, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(b == 'O');
    return 0;
}

/** 100× event overflow drop-oldest; queue still usable. */
int m20i_test_evt_overflow_repeat(void) {
    haeun_evt_hub hub;
    haeun_evt ev;
    int i;
    haeun_evt_hub_init(&hub, NULL);
    for (i = 0; i < 100; ++i) {
        ev.type = HAEUN_EVT_KEY_DOWN;
        ev.a = (uint32_t)i;
        CHECK(haeun_evt_push(&hub, 4, &ev) == 0);
    }
    CHECK(hub.slots[0].drops > 0u || hub.slots[0].count == HAEUN_EVT_QUEUE);
    CHECK(haeun_evt_poll(&hub, 4, &ev) == 1);
    return 0;
}

/** Service start/stop cycle ×40 (AS slot limited). */
int m20i_test_service_repeat(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_app_env apps;
    haeun_sm_hub sm;
    haeun_evt_hub evt;
    uint8_t appimg[256];
    size_t alen = 0u;
    int i;
    haeun_sm_stat st;
    m20i_prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    haeun_evt_hub_init(&evt, NULL);
    CHECK(haeun_sm_init(&sm, &vm, &apps, &evt) == HAEUN_SM_OK);
    m20i_make_tiny_hxf(appimg, &alen);
    CHECK(haeun_app_register(&apps, "svc", appimg, alen) == HAEUN_APP_OK);
    CHECK(haeun_sm_register(&sm, "svc", "d", "1", 0, 0, HAEUN_SM_RESTART_NEVER) ==
          HAEUN_SM_OK);
    for (i = 0; i < 40; ++i) {
        uint32_t pid;
        CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_OK);
        pid = sm.tab[0].pid;
        CHECK(haeun_sm_stop(&sm, 0, 1u) == HAEUN_SM_OK);
        m20i_wr32(ram + M20I_PCB_BASE + 1u * M20I_PCB_STRIDE, 0u);
        m20i_wr32(ram + M20I_PCB_BASE + 1u * M20I_PCB_STRIDE + 4u, 0u);
        (void)haeun_app_reap(&apps, pid);
        haeun_evt_reap(&evt, (int)pid);
        haeun_sm_on_pid_exit(&sm, pid, 0u, 0);
        CHECK(haeun_sm_status(&sm, 0, &st) == HAEUN_SM_OK);
        CHECK(st.state == HAEUN_SM_STOPPED);
    }
    return 0;
}
