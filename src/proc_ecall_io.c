#include "haeun/guest_pcb.h"
#include "haeun/mmu.h"
#include "haeun/proc_hfs.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stdio.h>
#include <string.h>

static void io_log(const char *detail) {
    fprintf(stderr, "[haeun-proc-hfs] failure: ecall_io %s\n",
            detail != NULL ? detail : "");
}

static bool guest_load_u8(haeun_vm *vm, uint32_t va, uint8_t *out) {
    uint32_t pa = 0u;
    uint32_t cause = 0u;
    if (vm == NULL || out == NULL)
        return false;
    if ((vm->csr_ptbr & 1u) == 0u) {
        if ((uint64_t)va >= (uint64_t)vm->ram_size)
            return false;
        *out = vm->ram[va];
        return true;
    }
    if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_R, &pa, &cause))
        return false;
    if ((uint64_t)pa >= (uint64_t)vm->ram_size)
        return false;
    *out = vm->ram[pa];
    return true;
}

static bool guest_store_u8(haeun_vm *vm, uint32_t va, uint8_t b) {
    uint32_t pa = 0u;
    uint32_t cause = 0u;
    if (vm == NULL)
        return false;
    if ((vm->csr_ptbr & 1u) == 0u) {
        if ((uint64_t)va >= (uint64_t)vm->ram_size)
            return false;
        vm->ram[va] = b;
        return true;
    }
    if (!haeun_mmu_translate(vm, va, HAEUN_MMU_ACCESS_W, &pa, &cause))
        return false;
    if ((uint64_t)pa >= (uint64_t)vm->ram_size)
        return false;
    vm->ram[pa] = b;
    return true;
}

static bool copy_guest_path(haeun_vm *vm, uint32_t va, char *out, size_t cap) {
    size_t i;
    if (out == NULL || cap == 0u)
        return false;
    for (i = 0u; i + 1u < cap; ++i) {
        uint8_t b = 0u;
        if (!guest_load_u8(vm, va + (uint32_t)i, &b))
            return false;
        out[i] = (char)b;
        if (b == 0u)
            return true;
    }
    out[cap - 1u] = '\0';
    return false;
}

static bool copy_to_guest(haeun_vm *vm, uint32_t va, const char *src,
                          uint32_t n) {
    uint32_t i;
    haeun_sec_syscall_req sreq;
    memset(&sreq, 0, sizeof(sreq));
    sreq.privilege = HAEUN_PRIV_USER;
    sreq.vm = vm;
    sreq.user_hi = (uint32_t)vm->ram_size;
    if (haeun_sec_check_user_va(&sreq, va, HAEUN_MMU_ACCESS_W) != HAEUN_SEC_OK)
        return false;
    for (i = 0u; i < n; ++i) {
        if (!guest_store_u8(vm, va + i, (uint8_t)src[i]))
            return false;
    }
    return true;
}

static uint32_t hfs_to_ret(haeun_hfs_result r, uint32_t ok_value) {
    if (r == HAEUN_HFS_OK)
        return ok_value;
    return 0xffffffffu;
}

static int fd_owner_ok(const haeun_proc_ft *ft, uint32_t fd, int pid) {
    if (ft == NULL || fd >= HAEUN_PROC_NFD)
        return 0;
    if (ft->fds[fd].kind == HAEUN_PROC_FD_FREE)
        return 0;
    if (ft->fds[fd].owner_pid == 0 || ft->fds[fd].owner_pid == pid)
        return 1;
    return 0;
}

/**
 * Completes process file syscalls on the host when a file table is installed.
 */
bool haeun_proc_ecall_io(haeun_vm *vm, void *context) {
    haeun_proc_ft *ft = (haeun_proc_ft *)context;
    uint32_t number, a0, a1, a2;
    uint32_t fd = 0u;
    uint32_t n = 0u;
    uint8_t byte = 0u;
    char path[HAEUN_HFS_NAME_MAX + 2u];
    char dirbuf[256];
    uint32_t got = 0u;
    uint32_t size = 0u;
    uint16_t mode = 0u;
    haeun_hfs_result r;
    int pid;
    if (vm == NULL || ft == NULL || vm->privilege != HAEUN_PRIV_USER)
        return false;
    number = haeun_rf_read(vm, 1u);
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    pid = (int)haeun_guest_pcb_current_pid(vm);
    switch (number) {
    case HAEUN_SYS_OPEN:
        if (a0 == 1u) {
            r = haeun_proc_open_console(ft, &fd);
            if (r == HAEUN_HFS_OK && pid != 0)
                (void)haeun_proc_fd_set_owner(ft, fd, pid);
            haeun_rf_write(vm, 1u, hfs_to_ret(r, fd));
            return true;
        }
        if (!copy_guest_path(vm, a0, path, sizeof(path))) {
            io_log("bad path va");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_open_path(ft, path, a1 != 0u ? 1u : 0u, &fd);
        if (r == HAEUN_HFS_OK && pid != 0)
            (void)haeun_proc_fd_set_owner(ft, fd, pid);
        haeun_rf_write(vm, 1u, hfs_to_ret(r, fd));
        return true;
    case HAEUN_SYS_CLOSE:
        if (pid != 0 && !fd_owner_ok(ft, a0, pid)) {
            io_log("close ownership denied");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_close(ft, a0);
        haeun_rf_write(vm, 1u, hfs_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_WRITE:
        if (a0 >= HAEUN_PROC_NFD || ft->fds[a0].kind != HAEUN_PROC_FD_FILE)
            return false;
        if (pid != 0 && !fd_owner_ok(ft, a0, pid)) {
            io_log("write ownership denied");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        byte = (uint8_t)(a1 & 0xffu);
        r = haeun_proc_write(ft, a0, &byte, 1u, &n);
        haeun_rf_write(vm, 1u, hfs_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_READ:
        if (a0 >= HAEUN_PROC_NFD || ft->fds[a0].kind != HAEUN_PROC_FD_FILE)
            return false;
        if (pid != 0 && !fd_owner_ok(ft, a0, pid)) {
            io_log("read ownership denied");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_read(ft, a0, &byte, 1u, &n);
        if (r != HAEUN_HFS_OK || n == 0u) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, (uint32_t)byte);
        return true;
    case HAEUN_SYS_FS_READDIR:
        /* r2=buf VA r3=cap → r1=bytes packed */
        if (a1 == 0u || a1 > sizeof(dirbuf)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_readdir_root(ft, dirbuf, a1, &got);
        if (r != HAEUN_HFS_OK || !copy_to_guest(vm, a0, dirbuf, got)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, got);
        return true;
    case HAEUN_SYS_FS_UNLINK:
        if (!copy_guest_path(vm, a0, path, sizeof(path))) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_unlink_root(ft, path);
        haeun_rf_write(vm, 1u, hfs_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_FS_STAT:
        if (!copy_guest_path(vm, a0, path, sizeof(path))) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_stat_root(ft, path, &size, &mode);
        if (r != HAEUN_HFS_OK) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        haeun_rf_write(vm, 1u, 0u);
        haeun_rf_write(vm, 2u, size);
        haeun_rf_write(vm, 3u, (uint32_t)mode);
        return true;
    case HAEUN_SYS_FS_SEEK:
        if (pid != 0 && !fd_owner_ok(ft, a0, pid)) {
            io_log("seek ownership denied");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_proc_seek(ft, a0, a1);
        haeun_rf_write(vm, 1u, hfs_to_ret(r, 0u));
        (void)a2;
        return true;
    default:
        return false;
    }
}
