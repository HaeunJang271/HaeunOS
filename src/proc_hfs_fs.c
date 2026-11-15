#include "haeun/proc_hfs.h"

#include <stdio.h>
#include <string.h>

static void fs_log(const char *detail) {
    fprintf(stderr, "[haeun-proc-hfs] failure: %s\n",
            detail != NULL ? detail : "");
}

static const char *basename_only(const char *path) {
    const char *p;
    if (path == NULL)
        return NULL;
    p = path;
    if (p[0] == '/' && p[1] != '\0')
        p++;
    if (strchr(p, '/') != NULL)
        return NULL;
    if (p[0] == '\0')
        return NULL;
    return p;
}

/**
 * Packs root directory basenames (NUL-separated) into out.
 */
haeun_hfs_result haeun_proc_readdir_root(haeun_proc_ft *ft, char *out,
                                         uint32_t cap, uint32_t *got) {
    if (ft == NULL || out == NULL || got == NULL || cap == 0u) {
        fs_log("readdir bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fs == NULL || !ft->fs->mounted) {
        fs_log("readdir no fs");
        return HAEUN_HFS_IO;
    }
    return haeun_hfs_readdir(ft->fs, HAEUN_HFS_ROOT_INO, out, cap, got);
}

/** Unlinks a root basename (regular files only). */
haeun_hfs_result haeun_proc_unlink_root(haeun_proc_ft *ft, const char *path) {
    const char *name;
    if (ft == NULL) {
        fs_log("unlink bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fs == NULL || !ft->fs->mounted) {
        fs_log("unlink no fs");
        return HAEUN_HFS_IO;
    }
    name = basename_only(path);
    if (name == NULL || strlen(name) > HAEUN_HFS_NAME_MAX) {
        fs_log("unlink bad path");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    return haeun_hfs_unlink(ft->fs, HAEUN_HFS_ROOT_INO, name);
}

/**
 * Looks up root basename; writes size_bytes and mode.
 */
haeun_hfs_result haeun_proc_stat_root(haeun_proc_ft *ft, const char *path,
                                      uint32_t *size_out, uint16_t *mode_out) {
    const char *name;
    uint32_t inum = 0u;
    haeun_hfs_inode in;
    haeun_hfs_result r;
    if (ft == NULL || size_out == NULL || mode_out == NULL) {
        fs_log("stat bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fs == NULL || !ft->fs->mounted) {
        fs_log("stat no fs");
        return HAEUN_HFS_IO;
    }
    name = basename_only(path);
    if (name == NULL || strlen(name) > HAEUN_HFS_NAME_MAX) {
        fs_log("stat bad path");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    r = haeun_hfs_lookup(ft->fs, HAEUN_HFS_ROOT_INO, name, &inum);
    if (r != HAEUN_HFS_OK)
        return r;
    r = haeun_hfs_read_inode(ft->fs, inum, &in);
    if (r != HAEUN_HFS_OK)
        return r;
    *size_out = in.size_bytes;
    *mode_out = in.mode;
    return HAEUN_HFS_OK;
}

/** Sets file fd offset (absolute). */
haeun_hfs_result haeun_proc_seek(haeun_proc_ft *ft, uint32_t fd,
                                 uint32_t offset) {
    haeun_hfs_inode in;
    haeun_hfs_result r;
    if (ft == NULL || fd >= HAEUN_PROC_NFD) {
        fs_log("seek bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind != HAEUN_PROC_FD_FILE || ft->fs == NULL) {
        fs_log("seek bad fd");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    r = haeun_hfs_read_inode(ft->fs, ft->fds[fd].inum, &in);
    if (r != HAEUN_HFS_OK)
        return r;
    if (offset > in.size_bytes) {
        fs_log("seek past eof");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    ft->fds[fd].offset = offset;
    return HAEUN_HFS_OK;
}
