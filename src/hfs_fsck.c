#include "haeun/hfs.h"
#include "haeun/hfs_internal.h"

#include <string.h>

/**
 * Rebuilds the data-block bitmap from live inodes and clears orphaned state.
 */
haeun_hfs_result haeun_hfs_fsck(uint8_t *image, size_t image_size,
                                uint32_t *fixes) {
    haeun_hfs fs;
    haeun_hfs_super sb;
    uint8_t *bm;
    uint32_t i;
    uint32_t f = 0;
    haeun_hfs_result st;
    if (fixes != NULL) *fixes = 0;
    st = haeun_hfs_load_super(image, image_size, &sb);
    if (st != HAEUN_HFS_OK) return st;
    memset(&fs, 0, sizeof(fs));
    fs.image = image;
    fs.image_size = image_size;
    fs.sb = sb;
    fs.mounted = 1;
    bm = haeun_hfs_block(&fs, sb.bitmap_lba);
    if (bm == NULL) return HAEUN_HFS_IO;
    memset(bm, 0, HAEUN_HFS_BLOCK_SIZE);
    for (i = 0; i < sb.data_start; ++i)
        haeun_hfs_bitmap_set(bm, i, 1);
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
                f += 1u;
                (void)haeun_hfs_write_inode(&fs, i, &in);
            } else {
                haeun_hfs_bitmap_set(bm, lba, 1);
            }
        }
    }
    sb.generation += 1u;
    sb.state = HAEUN_HFS_STATE_CLEAN;
    haeun_hfs_store_super(image, &sb);
    f += 1u;
    if (fixes != NULL) *fixes = f;
    return HAEUN_HFS_OK;
}
