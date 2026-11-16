#ifndef HAEUN_APP_ENV_H
#define HAEUN_APP_ENV_H

#include "haeun/evt.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    HAEUN_APP_MAX = 5,
    HAEUN_APP_NAME_MAX = 16,
    HAEUN_APP_DESC_MAX = 24,
    HAEUN_APP_VER_MAX = 8,
    HAEUN_APP_CAT_MAX = 8,
    HAEUN_APP_CODE_MAX = 4096u,
    HAEUN_APP_HEAP_SIZE = 4096u,
    HAEUN_APP_USER_VA = 0x00108000u,
    HAEUN_APP_HEAP_VA = 0x00109000u,
    HAEUN_APP_SLOT_CODE_PA0 = 0x00108000u,
    HAEUN_APP_SLOT_CODE_PA1 = 0x0010C000u,
    HAEUN_APP_SLOT_CODE_PA2 = 0x0010D000u,
    HAEUN_APP_SLOT_HEAP_PA0 = 0x0010E000u,
    HAEUN_APP_SLOT_HEAP_PA1 = 0x0010F000u,
    HAEUN_APP_SLOT_HEAP_PA2 = 0x00110000u,
    HAEUN_APP_SLOTS = 3,
    HAEUN_APP_FLAG_SINGLE = 1u,
    /* APP_INFO field ids (ECALL 38) */
    HAEUN_APP_FIELD_NAME = 0,
    HAEUN_APP_FIELD_DESC = 1,
    HAEUN_APP_FIELD_VER = 2,
    HAEUN_APP_FIELD_CAT = 3,
    HAEUN_APP_FIELD_FLAGS = 4,
    /* APP_CTRL ops (ECALL 40) */
    HAEUN_APP_CTRL_FOCUS = 0,
    HAEUN_APP_CTRL_CLOSE = 1
};

typedef enum {
    HAEUN_APP_OK = 0,
    HAEUN_APP_BAD_ARGUMENT,
    HAEUN_APP_INVALID,
    HAEUN_APP_NOSPC,
    HAEUN_APP_NOENT,
    HAEUN_APP_BUSY
} haeun_app_result;

typedef struct {
    int used;
    char name[HAEUN_APP_NAME_MAX];
    char desc[HAEUN_APP_DESC_MAX];
    char version[HAEUN_APP_VER_MAX];
    char category[HAEUN_APP_CAT_MAX];
    uint32_t flags;
    const uint8_t *image;
    size_t image_size;
} haeun_app_entry;

typedef struct {
    int used;
    int app_index;
    uint32_t pid;
    int slot;
} haeun_app_run;

typedef struct {
    haeun_vm *vm;
    haeun_gfx_wm *wm;
    haeun_evt_hub *evt;
    haeun_app_entry apps[HAEUN_APP_MAX];
    int app_count;
    haeun_app_run runs[HAEUN_APP_SLOTS];
    uint32_t next_pid;
} haeun_app_env;

typedef struct {
    uint32_t pid;
    int app_index;
    uint32_t state;
    uint32_t exit_code;
    int win_id;
} haeun_app_run_info;

/**
 * Binds the application environment to a process-v1 guest VM and optional WM.
 */
haeun_app_result haeun_app_env_init(haeun_app_env *env, haeun_vm *vm,
                                    haeun_gfx_wm *wm);

/**
 * Registers a named HXF application image (validated, not copied).
 * Duplicate names are rejected. Metadata defaults: empty desc/ver/cat, flags 0.
 */
haeun_app_result haeun_app_register(haeun_app_env *env, const char *name,
                                    const uint8_t *image, size_t image_size);

/**
 * Sets optional metadata for a registered index (display name unchanged).
 */
haeun_app_result haeun_app_set_meta(haeun_app_env *env, int index,
                                    const char *desc, const char *version,
                                    const char *category, uint32_t flags);

/** Returns the number of registered applications. */
int haeun_app_count(const haeun_app_env *env);

/**
 * Copies the application name at index into out (NUL-terminated).
 */
haeun_app_result haeun_app_name(const haeun_app_env *env, int index, char *out,
                                size_t out_cap);

/**
 * Copies an APP_INFO string field, or writes *flags_out for FIELD_FLAGS.
 */
haeun_app_result haeun_app_info(const haeun_app_env *env, int index, int field,
                                char *out, size_t out_cap, uint32_t *flags_out);

/**
 * Validates, loads into a free AS slot, and starts a READY process.
 * Writes the new pid on success. Single-instance apps return BUSY if running.
 */
haeun_app_result haeun_app_spawn(haeun_app_env *env, int index, uint32_t *pid);

/**
 * Fills run info for AS slot (0..SLOTS-1). Empty/free slots return NOENT.
 */
haeun_app_result haeun_app_run_at(const haeun_app_env *env, int slot,
                                  haeun_app_run_info *out);

/**
 * Parent-only focus (op FOCUS) or soft CLOSE_REQUEST (op CLOSE) for child pid.
 */
haeun_app_result haeun_app_ctrl(haeun_app_env *env, int op, uint32_t target_pid,
                                uint32_t caller_pid);

/**
 * Marks a slot free when its pid has exited; destroys windows, wipes code/stack,
 * and invalidates the slot user-code PTE (defense in depth for reuse).
 */
haeun_app_result haeun_app_reap(haeun_app_env *env, uint32_t pid);

/** Destroys every WM window owned by pid (pid must be nonzero). */
haeun_app_result haeun_app_destroy_windows(haeun_gfx_wm *wm, uint32_t pid);

/** Returns the code PA for an AS slot, or 0 if out of range. */
uint32_t haeun_app_slot_code_pa(int slot);

/**
 * Returns the fixed user-heap PA for an AS slot, or 0 if out of range.
 */
uint32_t haeun_app_slot_heap_pa(int slot);

/** Reads the guest L0 user-code PTE word for a slot (0 if invalid). */
uint32_t haeun_app_slot_user_pte(const haeun_app_env *env, int slot);

/**
 * Host ECALL assist for APP_COUNT/NAME/SPAWN/INFO/RUN/CTRL (24–26, 38–40).
 * context must be haeun_app_env*.
 */
bool haeun_app_ecall_io(haeun_vm *vm, void *context);

#endif
