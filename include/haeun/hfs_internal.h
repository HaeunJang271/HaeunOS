#ifndef HAEUN_HFS_INTERNAL_H
#define HAEUN_HFS_INTERNAL_H

#include "haeun/hfs.h"

uint8_t *haeun_hfs_block(haeun_hfs *fs, uint32_t lba);
void haeun_hfs_encode_inode(uint8_t *dst, const haeun_hfs_inode *in);
void haeun_hfs_decode_inode(const uint8_t *src, haeun_hfs_inode *out);
haeun_hfs_result haeun_hfs_load_super(const uint8_t *image, size_t image_size,
                                      haeun_hfs_super *sb);
void haeun_hfs_store_super(uint8_t *image, const haeun_hfs_super *sb);
int haeun_hfs_bitmap_get(const uint8_t *bm, uint32_t bit);
void haeun_hfs_bitmap_set(uint8_t *bm, uint32_t bit, int value);
haeun_hfs_result haeun_hfs_alloc_block(haeun_hfs *fs, uint32_t *out_lba);
haeun_hfs_result haeun_hfs_free_block(haeun_hfs *fs, uint32_t lba);
haeun_hfs_result haeun_hfs_alloc_inode(haeun_hfs *fs, uint32_t *out_inum);

#endif
