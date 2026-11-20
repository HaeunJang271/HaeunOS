#include "haeun/hfs.h"
#include "haeun/hfs_internal.h"

#include <string.h>

/**
 * Rebuilds the data-block bitmap from live inodes and clears orphaned state.
 * A clean, consistent image is left byte-for-byte unchanged.
 */
haeun_hfs_result haeun_hfs_fsck(uint8_t *image, size_t image_size,
                                uint32_t *fixes) {
    haeun_hfs fs;
    haeun_hfs_super sb;
    uint8_t expected[HAEUN_HFS_BLOCK_SIZE];
    uint8_t *bm;
    uint32_t i;
    uint32_t f = 0;
    haeun_hfs_result st;

    if (fixes != NULL) *fixes = 0;

    st = haeun_hfs_load_super(image, image_size, &sb);
    if (st != HAEUN_HFS_OK) return st;

    if (sb.state != HAEUN_HFS_STATE_CLEAN &&
        sb.state != HAEUN_HFS_STATE_DIRTY)
        return HAEUN_HFS_INVALID;

    if (sb.block_count > HAEUN_HFS_BLOCK_SIZE * 8u ||
        sb.inode_count >
            (sb.data_start - sb.inode_start) *
                (HAEUN_HFS_BLOCK_SIZE / HAEUN_HFS_INODE_SIZE))
        return HAEUN_HFS_INVALID;

    memset(&fs, 0, sizeof(fs));
    fs.image = image;
    fs.image_size = image_size;
    fs.sb = sb;
    fs.mounted = 1;

    bm = haeun_hfs_block(&fs, sb.bitmap_lba);
    if (bm == NULL) return HAEUN_HFS_IO;

    memset(expected, 0, sizeof(expected));
    for (i = 0; i < sb.data_start; ++i)
        haeun_hfs_bitmap_set(expected, i, 1);

    for (i = 1; i < sb.inode_count; ++i) {
        haeun_hfs_inode in;
        uint32_t d;

        st = haeun_hfs_read_inode(&fs, i, &in);
        if (st != HAEUN_HFS_OK) return st;

        if (in.nlink == 0u && in.mode == 0u) continue;

        for (d = 0; d < HAEUN_HFS_DIRECT; ++d) {
            uint32_t lba = in.direct[d];

            if (lba == 0u) continue;

            if (lba < sb.data_start || lba >= sb.block_count) {
                in.direct[d] = 0u;
                st = haeun_hfs_write_inode(&fs, i, &in);
                if (st != HAEUN_HFS_OK) return st;
                f += 1u;
            } else {
                haeun_hfs_bitmap_set(expected, lba, 1);
            }
        }
    }

    if (memcmp(bm, expected, sizeof(expected)) != 0) {
        memcpy(bm, expected, sizeof(expected));
        f += 1u;
    }

    if (sb.state == HAEUN_HFS_STATE_DIRTY)
        f += 1u;

    if (f != 0u) {
        sb.generation += 1u;
        sb.state = HAEUN_HFS_STATE_CLEAN;
        haeun_hfs_store_super(image, &sb);
    }

    if (fixes != NULL) *fixes = f;
    return HAEUN_HFS_OK;
}
