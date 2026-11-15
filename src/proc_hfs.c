#include "haeun/proc_hfs.h"

#include <stdio.h>
#include <string.h>

static void proc_log(const char *event, const char *detail) {
    const char *e = event != NULL ? event : "(null)";
    const char *d = detail != NULL ? detail : "";
    fprintf(stderr, "[haeun-proc-hfs] failure: %s %s\n", e, d);
}

static int alloc_fd(haeun_proc_ft *ft) {
    uint32_t i;
    for (i = 0u; i < HAEUN_PROC_NFD; ++i) {
        if (ft->fds[i].kind == HAEUN_PROC_FD_FREE) return (int)i;
    }
    return -1;
}

static const char *basename_only(const char *path) {
    const char *p;
    if (path == NULL) return NULL;
    p = path;
    if (p[0] == '/' && p[1] != '\0') p++;
    if (strchr(p, '/') != NULL) return NULL;
    if (p[0] == '\0') return NULL;
    return p;
}

/**
 * Initializes a process file table bound to an optional mounted volume.
 */
void haeun_proc_ft_init(haeun_proc_ft *ft, haeun_hfs *fs) {
    if (ft == NULL) return;
    memset(ft, 0, sizeof(*ft));
    ft->fs = fs;
}

/**
 * Opens the console device into a free fd slot.
 */
haeun_hfs_result haeun_proc_open_console(haeun_proc_ft *ft, uint32_t *out_fd) {
    int slot;
    if (ft == NULL || out_fd == NULL) {
        proc_log("open_console", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    slot = alloc_fd(ft);
    if (slot < 0) {
        proc_log("open_console", "no fd");
        return HAEUN_HFS_NOSPC;
    }
    ft->fds[slot].kind = HAEUN_PROC_FD_CONSOLE;
    ft->fds[slot].inum = 0u;
    ft->fds[slot].offset = 0u;
    *out_fd = (uint32_t)slot;
    return HAEUN_HFS_OK;
}

/**
 * Opens or creates a root basename on the attached HFS volume.
 */
haeun_hfs_result haeun_proc_open_path(haeun_proc_ft *ft, const char *path,
                                      uint32_t create, uint32_t *out_fd) {
    const char *name;
    uint32_t inum = 0u;
    int slot;
    haeun_hfs_result r;
    if (ft == NULL || out_fd == NULL) {
        proc_log("open_path", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fs == NULL || !ft->fs->mounted) {
        proc_log("open_path", "no mounted fs");
        return HAEUN_HFS_IO;
    }
    name = basename_only(path);
    if (name == NULL || strlen(name) > HAEUN_HFS_NAME_MAX) {
        proc_log("open_path", "bad path");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (strcmp(name, "console") == 0) return haeun_proc_open_console(ft, out_fd);
    slot = alloc_fd(ft);
    if (slot < 0) {
        proc_log("open_path", "no fd");
        return HAEUN_HFS_NOSPC;
    }
    r = haeun_hfs_lookup(ft->fs, HAEUN_HFS_ROOT_INO, name, &inum);
    if (r == HAEUN_HFS_NOENT) {
        if (create == 0u) {
            proc_log("open_path", "missing");
            return HAEUN_HFS_NOENT;
        }
        r = haeun_hfs_create(ft->fs, HAEUN_HFS_ROOT_INO, name, 0644u, &inum);
        if (r != HAEUN_HFS_OK) {
            proc_log("open_path", "create failed");
            return r;
        }
    } else if (r != HAEUN_HFS_OK) {
        proc_log("open_path", "lookup failed");
        return r;
    }
    ft->fds[slot].kind = HAEUN_PROC_FD_FILE;
    ft->fds[slot].inum = inum;
    ft->fds[slot].offset = 0u;
    *out_fd = (uint32_t)slot;
    return HAEUN_HFS_OK;
}

/**
 * Reads bytes from an HFS file fd and advances its offset.
 */
haeun_hfs_result haeun_proc_read(haeun_proc_ft *ft, uint32_t fd, uint8_t *buf,
                                 uint32_t len, uint32_t *got) {
    haeun_hfs_result r;
    uint32_t n = 0u;
    if (ft == NULL || buf == NULL || got == NULL || fd >= HAEUN_PROC_NFD) {
        proc_log("read", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind == HAEUN_PROC_FD_FREE) {
        proc_log("read", "bad fd");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind != HAEUN_PROC_FD_FILE || ft->fs == NULL) {
        proc_log("read", "not a file fd");
        return HAEUN_HFS_IO;
    }
    r = haeun_hfs_read(ft->fs, ft->fds[fd].inum, ft->fds[fd].offset, buf, len,
                       &n);
    if (r != HAEUN_HFS_OK) {
        proc_log("read", "hfs read failed");
        return r;
    }
    ft->fds[fd].offset += n;
    *got = n;
    return HAEUN_HFS_OK;
}

/**
 * Writes bytes to an HFS file fd and advances its offset.
 */
haeun_hfs_result haeun_proc_write(haeun_proc_ft *ft, uint32_t fd,
                                  const uint8_t *buf, uint32_t len,
                                  uint32_t *wrote) {
    haeun_hfs_result r;
    uint32_t n = 0u;
    if (ft == NULL || buf == NULL || wrote == NULL || fd >= HAEUN_PROC_NFD) {
        proc_log("write", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind == HAEUN_PROC_FD_FREE) {
        proc_log("write", "bad fd");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind != HAEUN_PROC_FD_FILE || ft->fs == NULL) {
        proc_log("write", "not a file fd");
        return HAEUN_HFS_IO;
    }
    r = haeun_hfs_write(ft->fs, ft->fds[fd].inum, ft->fds[fd].offset, buf, len,
                        &n);
    if (r != HAEUN_HFS_OK) {
        proc_log("write", "hfs write failed");
        return r;
    }
    ft->fds[fd].offset += n;
    *wrote = n;
    return HAEUN_HFS_OK;
}

/**
 * Closes an fd slot in the process file table.
 */
haeun_hfs_result haeun_proc_close(haeun_proc_ft *ft, uint32_t fd) {
    if (ft == NULL || fd >= HAEUN_PROC_NFD) {
        proc_log("close", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    if (ft->fds[fd].kind == HAEUN_PROC_FD_FREE) {
        proc_log("close", "bad fd");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    memset(&ft->fds[fd], 0, sizeof(ft->fds[fd]));
    return HAEUN_HFS_OK;
}

/** Sets owning process pid on an allocated fd (0 clears ownership). */
haeun_hfs_result haeun_proc_fd_set_owner(haeun_proc_ft *ft, uint32_t fd,
                                         int pid) {
    if (ft == NULL || fd >= HAEUN_PROC_NFD ||
        ft->fds[fd].kind == HAEUN_PROC_FD_FREE) {
        proc_log("set_owner", "bad fd");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    ft->fds[fd].owner_pid = pid;
    return HAEUN_HFS_OK;
}

/** Closes every fd whose owner_pid matches pid (pid must be nonzero). */
haeun_hfs_result haeun_proc_fds_close_by_owner(haeun_proc_ft *ft, int pid) {
    uint32_t i;
    if (ft == NULL || pid == 0) {
        proc_log("close_by_owner", "bad argument");
        return HAEUN_HFS_BAD_ARGUMENT;
    }
    for (i = 0u; i < HAEUN_PROC_NFD; ++i) {
        if (ft->fds[i].kind != HAEUN_PROC_FD_FREE &&
            ft->fds[i].owner_pid == pid)
            (void)haeun_proc_close(ft, i);
    }
    return HAEUN_HFS_OK;
}
