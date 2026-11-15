#ifndef HAEUN_PROC_HFS_H
#define HAEUN_PROC_HFS_H

#include "haeun/hfs.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

#define HAEUN_PROC_NFD 4u
#define HAEUN_PROC_FD_FREE 0u
#define HAEUN_PROC_FD_CONSOLE 1u
#define HAEUN_PROC_FD_FILE 2u

typedef struct {
    uint32_t kind;
    uint32_t inum;
    uint32_t offset;
    int owner_pid; /* 0 = unowned / legacy */
} haeun_proc_fd;

typedef struct {
    haeun_hfs *fs;
    haeun_proc_fd fds[HAEUN_PROC_NFD];
} haeun_proc_ft;

/**
 * Clears the fd table and attaches an optional mounted HFS (may be NULL).
 */
void haeun_proc_ft_init(haeun_proc_ft *ft, haeun_hfs *fs);

/**
 * Allocates a console fd (process-v1 device id 1 semantics).
 */
haeun_hfs_result haeun_proc_open_console(haeun_proc_ft *ft, uint32_t *out_fd);

/**
 * Opens a root-directory file by basename. If create!=0 and missing, creates it.
 */
haeun_hfs_result haeun_proc_open_path(haeun_proc_ft *ft, const char *path,
                                      uint32_t create, uint32_t *out_fd);

/**
 * Reads from an HFS-backed fd into buf; console fds return HAEUN_HFS_IO.
 */
haeun_hfs_result haeun_proc_read(haeun_proc_ft *ft, uint32_t fd, uint8_t *buf,
                                 uint32_t len, uint32_t *got);

/**
 * Writes to an HFS-backed fd from buf; advances the file offset.
 */
haeun_hfs_result haeun_proc_write(haeun_proc_ft *ft, uint32_t fd,
                                  const uint8_t *buf, uint32_t len,
                                  uint32_t *wrote);

/** Releases an fd slot. */
haeun_hfs_result haeun_proc_close(haeun_proc_ft *ft, uint32_t fd);

/**
 * Sets owning process pid on an allocated fd (0 clears ownership).
 */
haeun_hfs_result haeun_proc_fd_set_owner(haeun_proc_ft *ft, uint32_t fd,
                                         int pid);

/**
 * Closes every fd whose owner_pid matches pid (pid must be nonzero).
 */
haeun_hfs_result haeun_proc_fds_close_by_owner(haeun_proc_ft *ft, int pid);

/**
 * Packs root directory basenames (NUL-separated) into out.
 */
haeun_hfs_result haeun_proc_readdir_root(haeun_proc_ft *ft, char *out,
                                         uint32_t cap, uint32_t *got);

/** Unlinks a root basename (regular files only). */
haeun_hfs_result haeun_proc_unlink_root(haeun_proc_ft *ft, const char *path);

/**
 * Looks up root basename; writes size_bytes and mode (IFREG/IFDIR|perms).
 */
haeun_hfs_result haeun_proc_stat_root(haeun_proc_ft *ft, const char *path,
                                      uint32_t *size_out, uint16_t *mode_out);

/** Sets file fd offset (absolute). */
haeun_hfs_result haeun_proc_seek(haeun_proc_ft *ft, uint32_t fd,
                                 uint32_t offset);

/**
 * Host ECALL I/O assist for OPEN/READ/WRITE/CLOSE and FS_* (47–50).
 * OPEN: r2==1 console; else r2=path VA, r3!=0 create. Completes without trap.
 * Console READ/WRITE return false so the guest trap path can use UART.
 * context must be a haeun_proc_ft*.
 */
bool haeun_proc_ecall_io(haeun_vm *vm, void *context);

#endif
