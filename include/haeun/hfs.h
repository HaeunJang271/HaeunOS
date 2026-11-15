#ifndef HAEUN_HFS_H
#define HAEUN_HFS_H

#include <stddef.h>
#include <stdint.h>

#define HAEUN_HFS_MAGIC UINT32_C(0x31534648) /* HFS1 LE */
#define HAEUN_HFS_VERSION 1u
#define HAEUN_HFS_BLOCK_SIZE 512u
#define HAEUN_HFS_INODE_SIZE 64u
#define HAEUN_HFS_DIRENT_SIZE 32u
#define HAEUN_HFS_DIRECT 12u
#define HAEUN_HFS_NAME_MAX 27u
#define HAEUN_HFS_ROOT_INO 1u
#define HAEUN_HFS_IFREG 0x8000u
#define HAEUN_HFS_IFDIR 0x4000u
#define HAEUN_HFS_STATE_CLEAN 0u
#define HAEUN_HFS_STATE_DIRTY 1u

typedef enum {
    HAEUN_HFS_OK = 0,
    HAEUN_HFS_BAD_ARGUMENT,
    HAEUN_HFS_INVALID,
    HAEUN_HFS_NOSPC,
    HAEUN_HFS_NOENT,
    HAEUN_HFS_EXIST,
    HAEUN_HFS_NOTDIR,
    HAEUN_HFS_ISDIR,
    HAEUN_HFS_PERM,
    HAEUN_HFS_IO
} haeun_hfs_result;

typedef struct {
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t root_inum;
    uint32_t bitmap_lba;
    uint32_t inode_start;
    uint32_t data_start;
    uint32_t generation;
    uint32_t state;
} haeun_hfs_super;

typedef struct {
    uint16_t mode;
    uint16_t nlink;
    uint32_t size_bytes;
    uint32_t direct[HAEUN_HFS_DIRECT];
    uint16_t uid;
    uint16_t gid;
    uint32_t mtime;
} haeun_hfs_inode;

typedef struct {
    uint8_t *image;
    size_t image_size;
    haeun_hfs_super sb;
    int mounted;
} haeun_hfs;

/**
 * Formats caller-owned image bytes as a fresh HFS volume.
 * image_size must be a multiple of 512 and at least 16 blocks.
 */
haeun_hfs_result haeun_hfs_mkfs(uint8_t *image, size_t image_size,
                                uint32_t inode_count);

/** Loads and validates a superblock from image into fs (not yet dirty). */
haeun_hfs_result haeun_hfs_mount(haeun_hfs *fs, uint8_t *image,
                                 size_t image_size);

/** Marks the volume clean and clears the mount flag. */
haeun_hfs_result haeun_hfs_unmount(haeun_hfs *fs);

/** Repairs a dirty or inconsistent volume; bumps generation. */
haeun_hfs_result haeun_hfs_fsck(uint8_t *image, size_t image_size,
                                uint32_t *fixes);

haeun_hfs_result haeun_hfs_read_inode(haeun_hfs *fs, uint32_t inum,
                                      haeun_hfs_inode *out);
haeun_hfs_result haeun_hfs_write_inode(haeun_hfs *fs, uint32_t inum,
                                       const haeun_hfs_inode *in);

/** Looks up name in directory inum; writes child inum. */
haeun_hfs_result haeun_hfs_lookup(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name, uint32_t *out_inum);

haeun_hfs_result haeun_hfs_create(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name, uint16_t mode,
                                  uint32_t *out_inum);
haeun_hfs_result haeun_hfs_mkdir(haeun_hfs *fs, uint32_t dir_inum,
                                 const char *name, uint16_t mode,
                                 uint32_t *out_inum);
haeun_hfs_result haeun_hfs_unlink(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name);

haeun_hfs_result haeun_hfs_read(haeun_hfs *fs, uint32_t inum, uint32_t offset,
                                uint8_t *buf, uint32_t len, uint32_t *got);
haeun_hfs_result haeun_hfs_write(haeun_hfs *fs, uint32_t inum, uint32_t offset,
                                 const uint8_t *buf, uint32_t len,
                                 uint32_t *wrote);

/** Packs directory names into buf as NUL-separated list; writes byte count. */
haeun_hfs_result haeun_hfs_readdir(haeun_hfs *fs, uint32_t dir_inum,
                                   char *buf, uint32_t cap, uint32_t *got);

#endif
