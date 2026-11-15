#include "haeun/hfs.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int test_mkfs_create_persist_roundtrip(void) {
    uint8_t disk[128u * 1024u];
    uint8_t disk2[128u * 1024u];
    haeun_hfs fs;
    uint32_t ino = 0;
    uint32_t got = 0;
    uint8_t buf[64];
    const char *payload = "haeun-persist";
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 64u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_create(&fs, HAEUN_HFS_ROOT_INO, "note.txt", 0644, &ino) ==
          HAEUN_HFS_OK);
    CHECK(haeun_hfs_write(&fs, ino, 0, (const uint8_t *)payload,
                          (uint32_t)strlen(payload), &got) == HAEUN_HFS_OK);
    CHECK(got == strlen(payload));
    CHECK(haeun_hfs_mkdir(&fs, HAEUN_HFS_ROOT_INO, "home", 0755, &ino) ==
          HAEUN_HFS_OK);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    /* Simulate VM shutdown: keep only the raw image bytes. */
    memcpy(disk2, disk, sizeof(disk));
    memset(&fs, 0, sizeof(fs));
    CHECK(haeun_hfs_mount(&fs, disk2, sizeof(disk2)) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_lookup(&fs, HAEUN_HFS_ROOT_INO, "note.txt", &ino) ==
          HAEUN_HFS_OK);
    CHECK(haeun_hfs_read(&fs, ino, 0, buf, sizeof(buf), &got) == HAEUN_HFS_OK);
    CHECK(got == strlen(payload));
    CHECK(memcmp(buf, payload, got) == 0);
    CHECK(haeun_hfs_unlink(&fs, HAEUN_HFS_ROOT_INO, "note.txt") == HAEUN_HFS_OK);
    CHECK(haeun_hfs_lookup(&fs, HAEUN_HFS_ROOT_INO, "note.txt", &ino) ==
          HAEUN_HFS_NOENT);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

static int test_fsck_recovers_dirty(void) {
    uint8_t disk[64u * 1024u];
    haeun_hfs fs;
    uint32_t fixes = 0;
    uint32_t ino = 0;
    uint32_t wrote = 0;
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_create(&fs, HAEUN_HFS_ROOT_INO, "x", 0644, &ino) ==
          HAEUN_HFS_OK);
    CHECK(haeun_hfs_write(&fs, ino, 0, (const uint8_t *)"ab", 2u, &wrote) ==
          HAEUN_HFS_OK);
    /* Crash without unmount: leave dirty bit set. */
    CHECK(disk[36] == 1);
    CHECK(haeun_hfs_fsck(disk, sizeof(disk), &fixes) == HAEUN_HFS_OK);
    CHECK(fixes >= 1u);
    CHECK(disk[36] == 0);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_lookup(&fs, HAEUN_HFS_ROOT_INO, "x", &ino) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

static int test_permissions(void) {
    uint8_t disk[64u * 1024u];
    haeun_hfs fs;
    uint32_t ino = 0;
    uint32_t got = 0;
    uint8_t b;
    haeun_hfs_inode in;
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_create(&fs, HAEUN_HFS_ROOT_INO, "secret", 0000, &ino) ==
          HAEUN_HFS_OK);
    CHECK(haeun_hfs_read_inode(&fs, ino, &in) == HAEUN_HFS_OK);
    in.mode = (uint16_t)(HAEUN_HFS_IFREG | 0);
    CHECK(haeun_hfs_write_inode(&fs, ino, &in) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_read(&fs, ino, 0, &b, 1u, &got) == HAEUN_HFS_PERM);
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

int main(void) {
    if (test_mkfs_create_persist_roundtrip() != 0) return 1;
    if (test_fsck_recovers_dirty() != 0) return 1;
    if (test_permissions() != 0) return 1;
    puts("all HFS tests passed");
    return 0;
}
