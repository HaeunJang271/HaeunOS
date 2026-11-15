#ifndef HAEUN_SEC_H
#define HAEUN_SEC_H

#include "haeun/vm.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAEUN_SEC_OK = 0,
    HAEUN_SEC_DENIED,
    HAEUN_SEC_BAD_ARGUMENT
} haeun_sec_result;

enum {
    HAEUN_SYS_EXIT = 0,
    HAEUN_SYS_WRITE = 1,
    HAEUN_SYS_READ = 2,
    HAEUN_SYS_OPEN = 3,
    HAEUN_SYS_CLOSE = 4,
    HAEUN_SYS_FORK = 5,
    HAEUN_SYS_EXEC = 6,
    HAEUN_SYS_WAIT = 7,
    HAEUN_SYS_SLEEP = 8,
    HAEUN_SYS_GETPID = 9,
    HAEUN_SYS_YIELD = 10,
    HAEUN_SYS_SOCKET = 11,
    HAEUN_SYS_BIND = 12,
    HAEUN_SYS_SENDTO = 13,
    HAEUN_SYS_RECVFROM = 14,
    HAEUN_SYS_LISTEN = 15,
    HAEUN_SYS_ACCEPT = 16,
    HAEUN_SYS_SO_CLOSE = 17,
    HAEUN_SYS_WIN_CREATE = 18,
    HAEUN_SYS_WIN_DESTROY = 19,
    HAEUN_SYS_WIN_FILL = 20,
    HAEUN_SYS_WIN_PIXEL = 21,
    HAEUN_SYS_WIN_RAISE = 22,
    HAEUN_SYS_GFX_COMPOSE = 23,
    HAEUN_SYS_APP_COUNT = 24,
    HAEUN_SYS_APP_NAME = 25,
    HAEUN_SYS_APP_SPAWN = 26,
    /* M20 interactive userland */
    HAEUN_SYS_EVENT_POLL = 27,
    HAEUN_SYS_WIN_DRAW_TEXT = 28,
    HAEUN_SYS_IPC_CREATE = 29,
    HAEUN_SYS_IPC_SEND = 30,
    HAEUN_SYS_IPC_RECV = 31,
    HAEUN_SYS_IPC_CLOSE = 32,
    HAEUN_SYS_SVC_REGISTER = 33,
    HAEUN_SYS_SVC_LOOKUP = 34,
    /* M20.2: non-blocking queue query / metadata peek (29–32 ABI unchanged) */
    HAEUN_SYS_IPC_AVAILABLE = 35,
    HAEUN_SYS_IPC_PEEK = 36,
    /* M20.4: owner-only window geometry/state query */
    HAEUN_SYS_WIN_QUERY = 37,
    /* M20.5: launcher registry meta / run list / parent ctrl (additive) */
    HAEUN_SYS_APP_INFO = 38,
    HAEUN_SYS_APP_RUN = 39,
    HAEUN_SYS_APP_CTRL = 40,
    /* M20.6 Service Manager (process services; 33–34 remain IPC name registry) */
    HAEUN_SYS_SERVICE_COUNT = 41,
    HAEUN_SYS_SERVICE_INFO = 42,
    HAEUN_SYS_SERVICE_START = 43,
    HAEUN_SYS_SERVICE_STOP = 44,
    HAEUN_SYS_SERVICE_STATUS = 45,
    HAEUN_SYS_SERVICE_RESTART = 46,
    /* M20.7 File Manager HFS assists (OPEN/READ/WRITE/CLOSE unchanged) */
    HAEUN_SYS_FS_READDIR = 47,
    HAEUN_SYS_FS_UNLINK = 48,
    HAEUN_SYS_FS_STAT = 49,
    HAEUN_SYS_FS_SEEK = 50,
    HAEUN_SYS_MAX = 50
};

/**
 * Appends one security audit line to stderr (never silent on deny paths).
 */
void haeun_sec_log(const char *event, const char *detail);

/** Requires supervisor privilege; logs and denies otherwise. */
haeun_sec_result haeun_sec_require_supervisor(uint32_t privilege);

/**
 * Validates a syscall number for the caller privilege.
 * User may invoke 0..HAEUN_SYS_MAX; unknown numbers are denied.
 */
haeun_sec_result haeun_sec_validate_syscall(uint32_t privilege,
                                            uint32_t number);

/**
 * Rejects user-mode pointers that fall outside [lo, hi) (exclusive end).
 */
haeun_sec_result haeun_sec_check_user_range(uint32_t privilege, uint32_t addr,
                                            uint32_t length, uint32_t lo,
                                            uint32_t hi);

/**
 * Validates an HXF image header enough to refuse obvious forgeries before load.
 */
haeun_sec_result haeun_sec_validate_exec(const uint8_t *image, size_t size);

typedef struct {
    uint32_t privilege;
    uint32_t number;
    uint32_t arg0;
    uint32_t arg1;
    haeun_vm *vm; /* optional; enables MMU user-VA checks */
    uint32_t user_lo;
    uint32_t user_hi; /* exclusive flat window when vm paging is off */
    const uint8_t *exec_image;
    size_t exec_size;
} haeun_sec_syscall_req;

/**
 * Ensures a user VA is accessible for access_mask (R/W/X from mmu.h).
 * Supervisor always succeeds. With vm+paging, requires a successful translate
 * with U-capable mapping; otherwise uses [user_lo, user_hi).
 */
haeun_sec_result haeun_sec_check_user_va(const haeun_sec_syscall_req *req,
                                         uint32_t va, uint32_t access_mask);

/**
 * Applies Security v1/v2 gates for one syscall attempt; does not execute it.
 */
haeun_sec_result haeun_sec_dispatch(const haeun_sec_syscall_req *req);

/**
 * Validates the pending user ECALL against dispatch using live GPRs.
 */
haeun_sec_result haeun_sec_on_ecall(haeun_vm *vm);

/**
 * Runs the user ECALL gate before trap delivery. On deny, writes 0xffffffff
 * into r1 so process trap_entry takes the unknown-number path. Returns true
 * so the architectural trap is still entered (ISA v4 compatibility).
 */
bool haeun_sec_ecall_before_trap(haeun_vm *vm);

#endif


