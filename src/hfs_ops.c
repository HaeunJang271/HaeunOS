#include "haeun/hfs.h"
#include "haeun/hfs_internal.h"

#include <string.h>

static int name_ok(const char *name) {
    size_t n;
    if (name == NULL || name[0] == '\0') return 0;
    n = strlen(name);
    return n <= HAEUN_HFS_NAME_MAX && strchr(name, '/') == NULL;
}

static int can_write_dir(const haeun_hfs_inode *dir) {
    return (dir->mode & HAEUN_HFS_IFDIR) != 0 && (dir->mode & 0222u) != 0;
}

static int can_read_file(const haeun_hfs_inode *ino) {
    return (ino->mode & 0444u) != 0;
}

static int can_write_file(const haeun_hfs_inode *ino) {
    return (ino->mode & 0222u) != 0;
}

static haeun_hfs_result dir_find(haeun_hfs *fs, uint32_t dir_inum,
                                 const char *name, uint32_t *slot_lba,
                                 uint32_t *slot_off, uint32_t *inum) {
    haeun_hfs_inode dir;
    uint32_t b;
    uint8_t *blk;
    uint32_t off;
    haeun_hfs_result st = haeun_hfs_read_inode(fs, dir_inum, &dir);
    if (st != HAEUN_HFS_OK) return st;
    if ((dir.mode & HAEUN_HFS_IFDIR) == 0) return HAEUN_HFS_NOTDIR;
    for (b = 0; b < HAEUN_HFS_DIRECT; ++b) {
        if (dir.direct[b] == 0u) continue;
        blk = haeun_hfs_block(fs, dir.direct[b]);
        if (blk == NULL) return HAEUN_HFS_IO;
        for (off = 0; off < HAEUN_HFS_BLOCK_SIZE; off += HAEUN_HFS_DIRENT_SIZE) {
            uint32_t ent_ino = (uint32_t)blk[off] | ((uint32_t)blk[off + 1] << 8) |
                               ((uint32_t)blk[off + 2] << 16) |
                               ((uint32_t)blk[off + 3] << 24);
            if (ent_ino == 0u) continue;
            if (strncmp((const char *)(blk + off + 4), name, HAEUN_HFS_NAME_MAX) == 0) {
                if (slot_lba) *slot_lba = dir.direct[b];
                if (slot_off) *slot_off = off;
                if (inum) *inum = ent_ino;
                return HAEUN_HFS_OK;
            }
        }
    }
    return HAEUN_HFS_NOENT;
}

static haeun_hfs_result dir_add(haeun_hfs *fs, uint32_t dir_inum,
                                const char *name, uint32_t child) {
    haeun_hfs_inode dir;
    uint32_t b;
    uint8_t *blk;
    uint32_t off;
    uint32_t new_lba;
    haeun_hfs_result st = haeun_hfs_read_inode(fs, dir_inum, &dir);
    if (st != HAEUN_HFS_OK) return st;
    if (!can_write_dir(&dir)) return HAEUN_HFS_PERM;
    for (b = 0; b < HAEUN_HFS_DIRECT; ++b) {
        if (dir.direct[b] == 0u) {
            st = haeun_hfs_alloc_block(fs, &new_lba);
            if (st != HAEUN_HFS_OK) return st;
            dir.direct[b] = new_lba;
            dir.size_bytes += HAEUN_HFS_BLOCK_SIZE;
            st = haeun_hfs_write_inode(fs, dir_inum, &dir);
            if (st != HAEUN_HFS_OK) return st;
        }
        blk = haeun_hfs_block(fs, dir.direct[b]);
        if (blk == NULL) return HAEUN_HFS_IO;
        for (off = 0; off < HAEUN_HFS_BLOCK_SIZE; off += HAEUN_HFS_DIRENT_SIZE) {
            uint32_t ent_ino = (uint32_t)blk[off] | ((uint32_t)blk[off + 1] << 8) |
                               ((uint32_t)blk[off + 2] << 16) |
                               ((uint32_t)blk[off + 3] << 24);
            if (ent_ino != 0u) continue;
            blk[off] = (uint8_t)child;
            blk[off + 1] = (uint8_t)(child >> 8);
            blk[off + 2] = (uint8_t)(child >> 16);
            blk[off + 3] = (uint8_t)(child >> 24);
            memset(blk + off + 4, 0, 28);
            strncpy((char *)(blk + off + 4), name, HAEUN_HFS_NAME_MAX);
            return HAEUN_HFS_OK;
        }
    }
    return HAEUN_HFS_NOSPC;
}

haeun_hfs_result haeun_hfs_lookup(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name, uint32_t *out_inum) {
    if (!name_ok(name) || out_inum == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    return dir_find(fs, dir_inum, name, NULL, NULL, out_inum);
}

haeun_hfs_result haeun_hfs_create(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name, uint16_t mode,
                                  uint32_t *out_inum) {
    uint32_t exist;
    uint32_t inum;
    haeun_hfs_inode in;
    haeun_hfs_result st;
    if (!name_ok(name) || out_inum == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    if (dir_find(fs, dir_inum, name, NULL, NULL, &exist) == HAEUN_HFS_OK)
        return HAEUN_HFS_EXIST;
    st = haeun_hfs_alloc_inode(fs, &inum);
    if (st != HAEUN_HFS_OK) return st;
    memset(&in, 0, sizeof(in));
    in.mode = (uint16_t)(HAEUN_HFS_IFREG | (mode & 0777));
    in.nlink = 1u;
    in.mtime = 1u;
    st = haeun_hfs_write_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    st = dir_add(fs, dir_inum, name, inum);
    if (st != HAEUN_HFS_OK) return st;
    *out_inum = inum;
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_mkdir(haeun_hfs *fs, uint32_t dir_inum,
                                 const char *name, uint16_t mode,
                                 uint32_t *out_inum) {
    uint32_t exist;
    uint32_t inum;
    uint32_t lba;
    haeun_hfs_inode in;
    haeun_hfs_result st;
    if (!name_ok(name) || out_inum == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    if (dir_find(fs, dir_inum, name, NULL, NULL, &exist) == HAEUN_HFS_OK)
        return HAEUN_HFS_EXIST;
    st = haeun_hfs_alloc_inode(fs, &inum);
    if (st != HAEUN_HFS_OK) return st;
    st = haeun_hfs_alloc_block(fs, &lba);
    if (st != HAEUN_HFS_OK) return st;
    memset(&in, 0, sizeof(in));
    in.mode = (uint16_t)(HAEUN_HFS_IFDIR | (mode & 0777));
    in.nlink = 1u;
    in.size_bytes = HAEUN_HFS_BLOCK_SIZE;
    in.direct[0] = lba;
    in.mtime = 1u;
    st = haeun_hfs_write_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    st = dir_add(fs, dir_inum, name, inum);
    if (st != HAEUN_HFS_OK) return st;
    *out_inum = inum;
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_unlink(haeun_hfs *fs, uint32_t dir_inum,
                                  const char *name) {
    uint32_t lba;
    uint32_t off;
    uint32_t inum;
    haeun_hfs_inode in;
    uint8_t *blk;
    uint32_t i;
    haeun_hfs_result st;
    if (!name_ok(name)) return HAEUN_HFS_BAD_ARGUMENT;
    st = dir_find(fs, dir_inum, name, &lba, &off, &inum);
    if (st != HAEUN_HFS_OK) return st;
    st = haeun_hfs_read_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    if ((in.mode & HAEUN_HFS_IFDIR) != 0) return HAEUN_HFS_ISDIR;
    blk = haeun_hfs_block(fs, lba);
    if (blk == NULL) return HAEUN_HFS_IO;
    memset(blk + off, 0, HAEUN_HFS_DIRENT_SIZE);
    for (i = 0; i < HAEUN_HFS_DIRECT; ++i) {
        if (in.direct[i] != 0u) (void)haeun_hfs_free_block(fs, in.direct[i]);
        in.direct[i] = 0u;
    }
    in.nlink = 0;
    in.mode = 0;
    in.size_bytes = 0;
    return haeun_hfs_write_inode(fs, inum, &in);
}

haeun_hfs_result haeun_hfs_read(haeun_hfs *fs, uint32_t inum, uint32_t offset,
                                uint8_t *buf, uint32_t len, uint32_t *got) {
    haeun_hfs_inode in;
    uint32_t pos;
    uint32_t n;
    haeun_hfs_result st;
    if (buf == NULL || got == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    st = haeun_hfs_read_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    if ((in.mode & HAEUN_HFS_IFREG) == 0) return HAEUN_HFS_ISDIR;
    if (!can_read_file(&in)) return HAEUN_HFS_PERM;
    if (offset >= in.size_bytes) {
        *got = 0;
        return HAEUN_HFS_OK;
    }
    if (len > in.size_bytes - offset) len = in.size_bytes - offset;
    pos = 0;
    while (pos < len) {
        uint32_t blk_index = (offset + pos) / HAEUN_HFS_BLOCK_SIZE;
        uint32_t blk_off = (offset + pos) % HAEUN_HFS_BLOCK_SIZE;
        uint8_t *blk;
        if (blk_index >= HAEUN_HFS_DIRECT || in.direct[blk_index] == 0u)
            break;
        blk = haeun_hfs_block(fs, in.direct[blk_index]);
        if (blk == NULL) return HAEUN_HFS_IO;
        n = HAEUN_HFS_BLOCK_SIZE - blk_off;
        if (n > len - pos) n = len - pos;
        memcpy(buf + pos, blk + blk_off, n);
        pos += n;
    }
    *got = pos;
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_write(haeun_hfs *fs, uint32_t inum, uint32_t offset,
                                 const uint8_t *buf, uint32_t len,
                                 uint32_t *wrote) {
    haeun_hfs_inode in;
    uint32_t pos;
    uint32_t n;
    haeun_hfs_result st;
    if (buf == NULL || wrote == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    st = haeun_hfs_read_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    if ((in.mode & HAEUN_HFS_IFREG) == 0) return HAEUN_HFS_ISDIR;
    if (!can_write_file(&in)) return HAEUN_HFS_PERM;
    pos = 0;
    while (pos < len) {
        uint32_t blk_index = (offset + pos) / HAEUN_HFS_BLOCK_SIZE;
        uint32_t blk_off = (offset + pos) % HAEUN_HFS_BLOCK_SIZE;
        uint8_t *blk;
        if (blk_index >= HAEUN_HFS_DIRECT) return HAEUN_HFS_NOSPC;
        if (in.direct[blk_index] == 0u) {
            uint32_t lba;
            st = haeun_hfs_alloc_block(fs, &lba);
            if (st != HAEUN_HFS_OK) return st;
            in.direct[blk_index] = lba;
        }
        blk = haeun_hfs_block(fs, in.direct[blk_index]);
        if (blk == NULL) return HAEUN_HFS_IO;
        n = HAEUN_HFS_BLOCK_SIZE - blk_off;
        if (n > len - pos) n = len - pos;
        memcpy(blk + blk_off, buf + pos, n);
        pos += n;
    }
    if (offset + pos > in.size_bytes) in.size_bytes = offset + pos;
    in.mtime += 1u;
    st = haeun_hfs_write_inode(fs, inum, &in);
    if (st != HAEUN_HFS_OK) return st;
    *wrote = pos;
    return HAEUN_HFS_OK;
}

haeun_hfs_result haeun_hfs_readdir(haeun_hfs *fs, uint32_t dir_inum, char *buf,
                                   uint32_t cap, uint32_t *got) {
    haeun_hfs_inode dir;
    uint32_t b;
    uint32_t used = 0;
    haeun_hfs_result st;
    if (buf == NULL || got == NULL) return HAEUN_HFS_BAD_ARGUMENT;
    st = haeun_hfs_read_inode(fs, dir_inum, &dir);
    if (st != HAEUN_HFS_OK) return st;
    if ((dir.mode & HAEUN_HFS_IFDIR) == 0) return HAEUN_HFS_NOTDIR;
    for (b = 0; b < HAEUN_HFS_DIRECT; ++b) {
        uint8_t *blk;
        uint32_t off;
        if (dir.direct[b] == 0u) continue;
        blk = haeun_hfs_block(fs, dir.direct[b]);
        if (blk == NULL) return HAEUN_HFS_IO;
        for (off = 0; off < HAEUN_HFS_BLOCK_SIZE; off += HAEUN_HFS_DIRENT_SIZE) {
            uint32_t ent_ino = (uint32_t)blk[off] | ((uint32_t)blk[off + 1] << 8) |
                               ((uint32_t)blk[off + 2] << 16) |
                               ((uint32_t)blk[off + 3] << 24);
            size_t n;
            if (ent_ino == 0u) continue;
            n = 0;
            while (n < HAEUN_HFS_NAME_MAX && blk[off + 4u + n] != 0)
                n++;
            if (used + (uint32_t)n + 1u > cap) return HAEUN_HFS_NOSPC;
            memcpy(buf + used, blk + off + 4, n);
            used = (uint32_t)(used + n);
            buf[used++] = '\0';
        }
    }
    *got = used;
    return HAEUN_HFS_OK;
}
