#include "haeun/hfs.h"
#include "haeun/hfs_internal.h"

#include <string.h>

/**
 * Formats image as HFS with inode_count inodes (minimum 16).
 */
haeun_hfs_result haeun_hfs_mkfs(uint8_t *image, size_t image_size,
                                uint32_t inode_count) {
    haeun_hfs_super sb;
    haeun_hfs_inode root;
    uint32_t blocks;
    uint32_t per = HAEUN_HFS_BLOCK_SIZE / HAEUN_HFS_INODE_SIZE;
    uint32_t inode_blocks;
    uint32_t i;
    uint8_t *bm;
    uint8_t *iblk;
    if (image == NULL || image_size < 16u * HAEUN_HFS_BLOCK_SIZE ||
        (image_size % HAEUN_HFS_BLOCK_SIZE) != 0u || inode_count < 16u)
        return HAEUN_HFS_BAD_ARGUMENT;
    blocks = (uint32_t)(image_size / HAEUN_HFS_BLOCK_SIZE);
    if (inode_count > blocks * 2u) return HAEUN_HFS_BAD_ARGUMENT;
    inode_blocks = (inode_count + per - 1u) / per;
    memset(&sb, 0, sizeof(sb));
    sb.block_count = blocks;
    sb.inode_count = inode_count;
    sb.root_inum = HAEUN_HFS_ROOT_INO;
    sb.bitmap_lba = 1u;
    sb.inode_start = 2u;
    sb.data_start = 2u + inode_blocks;
    sb.generation = 1u;
    sb.state = HAEUN_HFS_STATE_CLEAN;
    if (sb.data_start >= blocks) return HAEUN_HFS_NOSPC;
    memset(image, 0, image_size);
    haeun_hfs_store_super(image, &sb);
    bm = image + HAEUN_HFS_BLOCK_SIZE;
    for (i = 0; i < sb.data_start; ++i)
        haeun_hfs_bitmap_set(bm, i, 1);
    memset(&root, 0, sizeof(root));
    root.mode = (uint16_t)(HAEUN_HFS_IFDIR | 0755);
    root.nlink = 1u;
    root.size_bytes = HAEUN_HFS_BLOCK_SIZE;
    root.direct[0] = sb.data_start;
    root.uid = 0;
    root.gid = 0;
    root.mtime = 1u;
    haeun_hfs_bitmap_set(bm, sb.data_start, 1);
    iblk = image + (size_t)sb.inode_start * HAEUN_HFS_BLOCK_SIZE;
    haeun_hfs_encode_inode(iblk + HAEUN_HFS_INODE_SIZE, &root);
    /* empty directory block already zero */
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_mount(haeun_hfs *fs, uint8_t *image,
                                 size_t image_size) {
    haeun_hfs_result st;
    if (fs == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    memset(fs, 0, sizeof(*fs));
    st = haeun_hfs_load_super(image, image_size, &fs->sb);
    if (st != HAEUN_HFS_OK) return st;
    fs->image = image;
    fs->image_size = image_size;
    fs->mounted = 1;
    fs->sb.state = HAEUN_HFS_STATE_DIRTY;
    haeun_hfs_store_super(fs->image, &fs->sb);
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_unmount(haeun_hfs *fs) {
    if (fs == NULL || !fs->mounted) return HAEUN_HFS_BAD_ARGUMENT;
    fs->sb.state = HAEUN_HFS_STATE_CLEAN;
    haeun_hfs_store_super(fs->image, &fs->sb);
    fs->mounted = 0;
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_alloc_block(haeun_hfs *fs, uint32_t *out_lba) {
    uint8_t *bm;
    uint32_t i;
    if (fs == NULL || out_lba == NULL || !fs->mounted) return HAEUN_HFS_BAD_ARGUMENT;
    bm = haeun_hfs_block(fs, fs->sb.bitmap_lba);
    if (bm == NULL) return HAEUN_HFS_IO;
    for (i = fs->sb.data_start; i < fs->sb.block_count; ++i) {
        if (!haeun_hfs_bitmap_get(bm, i)) {
            haeun_hfs_bitmap_set(bm, i, 1);
            memset(haeun_hfs_block(fs, i), 0, HAEUN_HFS_BLOCK_SIZE);
            *out_lba = i;
            fs->sb.state = HAEUN_HFS_STATE_DIRTY;
            haeun_hfs_store_super(fs->image, &fs->sb);
            return HAEUN_HFS_OK;
        }
    }
    return HAEUN_HFS_NOSPC;
}

haeun_hfs_result haeun_hfs_free_block(haeun_hfs *fs, uint32_t lba) {
    uint8_t *bm;
    if (fs == NULL || !fs->mounted) return HAEUN_HFS_BAD_ARGUMENT;
    if (lba < fs->sb.data_start || lba >= fs->sb.block_count)
        return HAEUN_HFS_INVALID;
    bm = haeun_hfs_block(fs, fs->sb.bitmap_lba);
    if (bm == NULL) return HAEUN_HFS_IO;
    haeun_hfs_bitmap_set(bm, lba, 0);
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_alloc_inode(haeun_hfs *fs, uint32_t *out_inum) {
    uint32_t i;
    haeun_hfs_inode in;
    if (fs == NULL || out_inum == NULL || !fs->mounted) return HAEUN_HFS_BAD_ARGUMENT;
    for (i = 2u; i < fs->sb.inode_count; ++i) {
        if (haeun_hfs_read_inode(fs, i, &in) != HAEUN_HFS_OK) return HAEUN_HFS_IO;
        if (in.nlink == 0u && in.mode == 0u) {
            *out_inum = i;
            return HAEUN_HFS_OK;
        }
    }
    return HAEUN_HFS_NOSPC;
}
