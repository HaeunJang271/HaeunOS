#include "haeun/hfs.h"
#include "haeun/proc_hfs.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
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

static int test_roundtrip(void) {
    uint8_t img[32u * 512u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd = 0u, fd2 = 0u, n = 0u, size = 0u, got = 0u;
    uint16_t mode = 0u;
    uint8_t b = 0u;
    char names[128];
    CHECK(haeun_hfs_mkfs(img, sizeof(img), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, img, sizeof(img)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "note", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, fd, 3) == HAEUN_HFS_OK);
    CHECK(haeun_proc_write(&ft, fd, (const uint8_t *)"AB", 2u, &n) ==
          HAEUN_HFS_OK);
    CHECK(n == 2u);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_stat_root(&ft, "note", &size, &mode) == HAEUN_HFS_OK);
    CHECK(size == 2u);
    CHECK((mode & HAEUN_HFS_IFREG) != 0u);
    CHECK(haeun_proc_open_path(&ft, "note", 0u, &fd2) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, fd2, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(b == 'A');
    CHECK(haeun_proc_seek(&ft, fd2, 0u) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, fd2, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(b == 'A');
    CHECK(haeun_proc_close(&ft, fd2) == HAEUN_HFS_OK);
    CHECK(haeun_proc_readdir_root(&ft, names, sizeof(names), &got) ==
          HAEUN_HFS_OK);
    CHECK(got >= 5u);
    CHECK(strstr(names, "note") != NULL);
    CHECK(haeun_proc_unlink_root(&ft, "note") == HAEUN_HFS_OK);
    CHECK(haeun_proc_stat_root(&ft, "note", &size, &mode) == HAEUN_HFS_NOENT);
    CHECK(haeun_proc_unlink_root(&ft, "missing") == HAEUN_HFS_NOENT);
    CHECK(haeun_proc_unlink_root(&ft, "a/b") == HAEUN_HFS_BAD_ARGUMENT);
    return 0;
}

static int test_fd_ownership(void) {
    uint8_t img[32u * 512u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    haeun_vm vm;
    uint8_t ram[64 * 1024];
    uint32_t fd = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_hfs_mkfs(img, sizeof(img), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, img, sizeof(img)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "x", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_fd_set_owner(&ft, fd, 5) == HAEUN_HFS_OK);
    vm.privilege = HAEUN_PRIV_USER;
    /* Fake current pid=7 via PCB bank */
    ram[0xA400] = 0;
    ram[0xA004] = 7;
    ram[0xA005] = 0;
    ram[0xA006] = 0;
    ram[0xA007] = 0;
    haeun_rf_write(&vm, 1u, HAEUN_SYS_FS_SEEK);
    haeun_rf_write(&vm, 2u, fd);
    haeun_rf_write(&vm, 3u, 0u);
    CHECK(haeun_proc_ecall_io(&vm, &ft));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    CHECK(haeun_proc_fds_close_by_owner(&ft, 5) == HAEUN_HFS_OK);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_FREE);
    return 0;
}

static int test_long_name_and_exist(void) {
    uint8_t img[32u * 512u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd = 0u;
    char longn[40];
    memset(longn, 'a', sizeof(longn) - 1u);
    longn[sizeof(longn) - 1u] = '\0';
    CHECK(haeun_hfs_mkfs(img, sizeof(img), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, img, sizeof(img)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, longn, 1u, &fd) == HAEUN_HFS_BAD_ARGUMENT);
    CHECK(haeun_proc_open_path(&ft, "a", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_create(&fs, HAEUN_HFS_ROOT_INO, "a", 0644u, &fd) ==
          HAEUN_HFS_EXIST);
    return 0;
}

int main(void) {
    if (test_roundtrip() != 0)
        return 1;
    if (test_fd_ownership() != 0)
        return 1;
    if (test_long_name_and_exist() != 0)
        return 1;
    puts("m20_file_tests ok");
    return 0;
}
