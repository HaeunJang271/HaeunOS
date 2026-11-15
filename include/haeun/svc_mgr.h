#ifndef HAEUN_SVC_MGR_H
#define HAEUN_SVC_MGR_H

#include "haeun/app_env.h"
#include "haeun/evt.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    HAEUN_SM_MAX = 4,
    HAEUN_SM_NAME_MAX = 16,
    HAEUN_SM_DESC_MAX = 24,
    HAEUN_SM_VER_MAX = 8,
    HAEUN_SM_MAX_RESTARTS = 3u,
    /* startup_type */
    HAEUN_SM_START_MANUAL = 0u,
    HAEUN_SM_START_AUTO = 1u,
    /* restart_policy */
    HAEUN_SM_RESTART_NEVER = 0u,
    HAEUN_SM_RESTART_ON_FAIL = 1u,
    HAEUN_SM_RESTART_ALWAYS = 2u,
    /* lifecycle state (must track real PCB) */
    HAEUN_SM_STOPPED = 0u,
    HAEUN_SM_STARTING = 1u,
    HAEUN_SM_RUNNING = 2u,
    HAEUN_SM_STOPPING = 3u,
    HAEUN_SM_FAILED = 4u,
    /* SERVICE_INFO fields */
    HAEUN_SM_FIELD_NAME = 0,
    HAEUN_SM_FIELD_DESC = 1,
    HAEUN_SM_FIELD_VER = 2,
    HAEUN_SM_FIELD_FLAGS = 3
};

typedef enum {
    HAEUN_SM_OK = 0,
    HAEUN_SM_BAD_ARGUMENT,
    HAEUN_SM_NOENT,
    HAEUN_SM_INVALID,
    HAEUN_SM_BUSY,
    HAEUN_SM_NOSPC,
    HAEUN_SM_FAILED_RC
} haeun_sm_result;

typedef struct {
    int used;
    char name[HAEUN_SM_NAME_MAX];
    char desc[HAEUN_SM_DESC_MAX];
    char version[HAEUN_SM_VER_MAX];
    int app_index;
    uint32_t startup_type;
    uint32_t restart_policy;
    uint32_t state;
    uint32_t pid;
    uint32_t run_count;
    uint32_t last_exit;
    uint32_t fail_count;
    uint32_t restart_count;
    uint32_t max_restarts;
    uint32_t generation; /* bumps on each start; detects PID reuse */
    uint32_t pending_restart; /* set on exit; applied when AS/PCB free */
} haeun_sm_entry;

typedef struct {
    haeun_vm *vm;
    haeun_app_env *apps;
    haeun_evt_hub *evt;
    haeun_sm_entry tab[HAEUN_SM_MAX];
    int count;
} haeun_sm_hub;

typedef struct {
    uint32_t state;
    uint32_t pid;
    uint32_t last_exit;
    uint32_t run_count;
    uint32_t fail_count;
    uint32_t restart_count;
    int app_index;
} haeun_sm_stat;

/**
 * Binds the service manager to the app environment (spawn/reap path).
 */
haeun_sm_result haeun_sm_init(haeun_sm_hub *hub, haeun_vm *vm,
                              haeun_app_env *apps, haeun_evt_hub *evt);

/**
 * Registers a service bound to an existing app registry index.
 * Does not spawn; validates the app slot exists.
 */
haeun_sm_result haeun_sm_register(haeun_sm_hub *hub, const char *name,
                                  const char *desc, const char *version,
                                  int app_index, uint32_t startup_type,
                                  uint32_t restart_policy);

/** Returns registered service count. */
int haeun_sm_count(const haeun_sm_hub *hub);

/** Copies a string INFO field, or packs flags into *flags_out. */
haeun_sm_result haeun_sm_info(const haeun_sm_hub *hub, int id, int field,
                              char *out, uint32_t out_cap, uint32_t *flags_out);

/**
 * Spawns the service's app; links PID. Caller must be able to parent the child
 * (APP_SPAWN uses current PCB pid).
 */
haeun_sm_result haeun_sm_start(haeun_sm_hub *hub, int id);

/**
 * Requests cooperative stop (event CLOSE + optional window CLOSE); STOPPING.
 * Does not poke PCB state.
 */
haeun_sm_result haeun_sm_stop(haeun_sm_hub *hub, int id, uint32_t caller_pid);

/** Syncs with PCB and fills status (real process state). */
haeun_sm_result haeun_sm_status(haeun_sm_hub *hub, int id, haeun_sm_stat *out);

/** Stop (if running) then start; respects restart counters. */
haeun_sm_result haeun_sm_restart(haeun_sm_hub *hub, int id,
                                 uint32_t caller_pid);

/**
 * Called when a pid exits/faults (mux reap). Updates service state and may
 * apply restart policy immediately (no host delay timer in v1).
 */
void haeun_sm_on_pid_exit(haeun_sm_hub *hub, uint32_t pid, uint32_t exit_code,
                          int was_fault);

/** Starts all AUTO services that are STOPPED (boot assist). */
void haeun_sm_autostart(haeun_sm_hub *hub);

/**
 * Host ECALL assist for SERVICE_* (41–46). context is haeun_sm_hub*.
 */
bool haeun_sm_ecall_io(haeun_vm *vm, void *context);

#endif
