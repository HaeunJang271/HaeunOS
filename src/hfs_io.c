#include "haeun/hfs.h"

#include <string.h>

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

uint8_t *haeun_hfs_block(haeun_hfs *fs, uint32_t lba) {
    if (fs == NULL || fs->image == NULL) return NULL;
    if ((uint64_t)lba * HAEUN_HFS_BLOCK_SIZE + HAEUN_HFS_BLOCK_SIZE >
        (uint64_t)fs->image_size)
        return NULL;
    return fs->image + (size_t)lba * HAEUN_HFS_BLOCK_SIZE;
}

void haeun_hfs_encode_inode(uint8_t *dst, const haeun_hfs_inode *in) {
    uint32_t i;
    wr16(dst + 0, in->mode);
    wr16(dst + 2, in->nlink);
    wr32(dst + 4, in->size_bytes);
    for (i = 0; i < HAEUN_HFS_DIRECT; ++i)
        wr32(dst + 8u + i * 4u, in->direct[i]);
    wr16(dst + 56, in->uid);
    wr16(dst + 58, in->gid);
    wr32(dst + 60, in->mtime);
}

void haeun_hfs_decode_inode(const uint8_t *src, haeun_hfs_inode *out) {
    uint32_t i;
    out->mode = rd16(src + 0);
    out->nlink = rd16(src + 2);
    out->size_bytes = rd32(src + 4);
    for (i = 0; i < HAEUN_HFS_DIRECT; ++i)
        out->direct[i] = rd32(src + 8u + i * 4u);
    out->uid = rd16(src + 56);
    out->gid = rd16(src + 58);
    out->mtime = rd32(src + 60);
}

haeun_hfs_result haeun_hfs_load_super(const uint8_t *image, size_t image_size,
                                      haeun_hfs_super *sb) {
    uint32_t blocks;
    if (image == NULL || sb == NULL || image_size < 16u * HAEUN_HFS_BLOCK_SIZE ||
        (image_size % HAEUN_HFS_BLOCK_SIZE) != 0u)
        return HAEUN_HFS_BAD_ARGUMENT;
    if (rd32(image + 0) != HAEUN_HFS_MAGIC || rd16(image + 4) != HAEUN_HFS_VERSION ||
        rd16(image + 6) != HAEUN_HFS_BLOCK_SIZE)
        return HAEUN_HFS_INVALID;
    blocks = (uint32_t)(image_size / HAEUN_HFS_BLOCK_SIZE);
    sb->block_count = rd32(image + 8);
    sb->inode_count = rd32(image + 12);
    sb->root_inum = rd32(image + 16);
    sb->bitmap_lba = rd32(image + 20);
    sb->inode_start = rd32(image + 24);
    sb->data_start = rd32(image + 28);
    sb->generation = rd32(image + 32);
    sb->state = rd32(image + 36);
    if (sb->block_count != blocks || sb->inode_count < 2u ||
        sb->root_inum != HAEUN_HFS_ROOT_INO || sb->bitmap_lba != 1u ||
        sb->inode_start < 2u || sb->data_start <= sb->inode_start ||
        sb->data_start >= sb->block_count)
        return HAEUN_HFS_INVALID;
    return HAEUN_HFS_OK;
}

void haeun_hfs_store_super(uint8_t *image, const haeun_hfs_super *sb) {
    memset(image, 0, HAEUN_HFS_BLOCK_SIZE);
    wr32(image + 0, HAEUN_HFS_MAGIC);
    wr16(image + 4, (uint16_t)HAEUN_HFS_VERSION);
    wr16(image + 6, (uint16_t)HAEUN_HFS_BLOCK_SIZE);
    wr32(image + 8, sb->block_count);
    wr32(image + 12, sb->inode_count);
    wr32(image + 16, sb->root_inum);
    wr32(image + 20, sb->bitmap_lba);
    wr32(image + 24, sb->inode_start);
    wr32(image + 28, sb->data_start);
    wr32(image + 32, sb->generation);
    wr32(image + 36, sb->state);
}

int haeun_hfs_bitmap_get(const uint8_t *bm, uint32_t bit) {
    return (bm[bit / 8u] >> (bit % 8u)) & 1;
}

void haeun_hfs_bitmap_set(uint8_t *bm, uint32_t bit, int value) {
    uint8_t mask = (uint8_t)(1u << (bit % 8u));
    if (value)
        bm[bit / 8u] = (uint8_t)(bm[bit / 8u] | mask);
    else
        bm[bit / 8u] = (uint8_t)(bm[bit / 8u] & (uint8_t)~mask);
}

haeun_hfs_result haeun_hfs_read_inode(haeun_hfs *fs, uint32_t inum,
                                      haeun_hfs_inode *out) {
    uint32_t per = HAEUN_HFS_BLOCK_SIZE / HAEUN_HFS_INODE_SIZE;
    uint32_t lba;
    uint8_t *blk;
    if (fs == NULL || out == NULL || !fs->mounted || inum == 0u ||
        inum >= fs->sb.inode_count)
        return HAEUN_HFS_BAD_ARGUMENT;
    lba = fs->sb.inode_start + (inum / per);
    blk = haeun_hfs_block(fs, lba);
    if (blk == NULL) return HAEUN_HFS_IO;
    haeun_hfs_decode_inode(blk + (inum % per) * HAEUN_HFS_INODE_SIZE, out);
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_write_inode(haeun_hfs *fs, uint32_t inum,
                                       const haeun_hfs_inode *in) {
    uint32_t per = HAEUN_HFS_BLOCK_SIZE / HAEUN_HFS_INODE_SIZE;
    uint32_t lba;
    uint8_t *blk;
    if (fs == NULL || in == NULL || !fs->mounted || inum == 0u ||
        inum >= fs->sb.inode_count)
        return HAEUN_HFS_BAD_ARGUMENT;
    lba = fs->sb.inode_start + (inum / per);
    blk = haeun_hfs_block(fs, lba);
    if (blk == NULL) return HAEUN_HFS_IO;
    haeun_hfs_encode_inode(blk + (inum % per) * HAEUN_HFS_INODE_SIZE, in);
    fs->sb.state = HAEUN_HFS_STATE_DIRTY;
    haeun_hfs_store_super(fs->image, &fs->sb);
    return HAEUN_HFS_OK;
}
