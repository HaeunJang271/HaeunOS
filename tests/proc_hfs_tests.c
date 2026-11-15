#include "haeun/hfs.h"
#include "haeun/proc_hfs.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * Console open/close uses process-v1 fd slots without touching HFS.
 */
static int test_console_fd(void) {
    haeun_proc_ft ft;
    uint32_t fd = 99u;
    haeun_proc_ft_init(&ft, NULL);
    CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(fd < HAEUN_PROC_NFD);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_CONSOLE);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_FREE);
    return 0;
}

/**
 * Shell-style create/write/close/reopen/read round-trip on HFS.
 */
static int test_file_roundtrip(void) {
    uint8_t disk[64u * 1024u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd = 0u;
    uint32_t n = 0u;
    uint8_t buf[32];
    const char *payload = "shell-note";
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "note.txt", 1u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_write(&ft, fd, (const uint8_t *)payload,
                           (uint32_t)strlen(payload), &n) == HAEUN_HFS_OK);
    CHECK(n == strlen(payload));
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_path(&ft, "/note.txt", 0u, &fd) == HAEUN_HFS_OK);
    memset(buf, 0, sizeof(buf));
    CHECK(haeun_proc_read(&ft, fd, buf, sizeof(buf), &n) == HAEUN_HFS_OK);
    CHECK(n == strlen(payload));
    CHECK(memcmp(buf, payload, n) == 0);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

/**
 * Missing file without create, nested path, and fd exhaustion are denied.
 */
static int test_open_failures(void) {
    uint8_t disk[64u * 1024u];
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd = 0u;
    uint32_t i;
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_proc_open_path(&ft, "missing", 0u, &fd) == HAEUN_HFS_NOENT);
    CHECK(haeun_proc_open_path(&ft, "a/b", 1u, &fd) == HAEUN_HFS_BAD_ARGUMENT);
    for (i = 0u; i < HAEUN_PROC_NFD; ++i)
        CHECK(haeun_proc_open_console(&ft, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_path(&ft, "x", 1u, &fd) == HAEUN_HFS_NOSPC);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

int main(void) {
    if (test_console_fd() != 0) return 1;
    if (test_file_roundtrip() != 0) return 1;
    if (test_open_failures() != 0) return 1;
    puts("all proc-hfs tests passed");
    return 0;
}
