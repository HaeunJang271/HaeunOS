#include "haeun/app_env.h"
#include "haeun/guest_pcb.h"
#include "haeun/hxf.h"
#include "haeun/sec.h"
#include "haeun/tlb.h"

#include <stdio.h>
#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_NEXT_PID = 0x0000A404u,
    L0_PTE_USER = 0x420u,
    L0_PTE_HEAP = 0x424u,
    PTE_FLAGS = 0x1bu,
    PTE_HEAP_FLAGS = 0x17u,
    STATE_FREE = 0,
    STATE_READY = 1,
    STATE_ZOMBIE = 5
};

static const uint32_t k_code_pa[HAEUN_APP_SLOTS] = {
    HAEUN_APP_SLOT_CODE_PA0, HAEUN_APP_SLOT_CODE_PA1, HAEUN_APP_SLOT_CODE_PA2};
static const uint32_t k_heap_pa[HAEUN_APP_SLOTS] = {
    HAEUN_APP_SLOT_HEAP_PA0, HAEUN_APP_SLOT_HEAP_PA1, HAEUN_APP_SLOT_HEAP_PA2};
static const uint32_t k_l0[HAEUN_APP_SLOTS] = {0x00003000u, 0x00007000u,
                                              0x00009000u};
static const uint32_t k_ptbr[HAEUN_APP_SLOTS] = {0x00002001u, 0x00006001u,
                                                0x00008001u};
static const uint32_t k_kstack[HAEUN_APP_SLOTS] = {0x00124000u, 0x00125000u,
                                                  0x00126000u};
static const uint32_t k_ustack_top[HAEUN_APP_SLOTS] = {0x00114000u, 0x00115000u,
                                                      0x00116000u};
static const uint32_t k_ustack_base[HAEUN_APP_SLOTS] = {0x00113000u, 0x00114000u,
                                                       0x00115000u};
static const uint32_t k_ustack_page[HAEUN_APP_SLOTS] = {0x00113000u, 0x00114000u,
                                                       0x00115000u};

static void app_log(const char *what) {
    fprintf(stderr, "[haeun-app] %s\n", what != NULL ? what : "");
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/** Returns the code PA for an AS slot, or 0 if out of range. */
uint32_t haeun_app_slot_code_pa(int slot) {
    if (slot < 0 || slot >= HAEUN_APP_SLOTS)
        return 0u;
    return k_code_pa[slot];
}

/** Returns the fixed user-heap PA for an AS slot, or 0 if out of range. */
uint32_t haeun_app_slot_heap_pa(int slot) {
    if (slot < 0 || slot >= HAEUN_APP_SLOTS)
        return 0u;
    return k_heap_pa[slot];
}

/** Reads the guest L0 user-code PTE word for a slot (0 if invalid). */
uint32_t haeun_app_slot_user_pte(const haeun_app_env *env, int slot) {
    if (env == NULL || env->vm == NULL || env->vm->ram == NULL || slot < 0 ||
        slot >= HAEUN_APP_SLOTS)
        return 0u;
    if ((uint64_t)k_l0[slot] + L0_PTE_USER + 4u > env->vm->ram_size)
        return 0u;
    return rd32(env->vm->ram + k_l0[slot] + L0_PTE_USER);
}

static void scrub_slot(haeun_app_env *env, int slot) {
    uint8_t *ram;
    if (env == NULL || env->vm == NULL || slot < 1 || slot >= HAEUN_APP_SLOTS)
        return;
    ram = env->vm->ram;
    if ((uint64_t)k_code_pa[slot] + HAEUN_APP_CODE_MAX <= env->vm->ram_size)
        memset(ram + k_code_pa[slot], 0, HAEUN_APP_CODE_MAX);
    if ((uint64_t)k_ustack_page[slot] + 4096u <= env->vm->ram_size)
        memset(ram + k_ustack_page[slot], 0, 4096u);
    if ((uint64_t)k_heap_pa[slot] + HAEUN_APP_HEAP_SIZE <= env->vm->ram_size)
        memset(ram + k_heap_pa[slot], 0, HAEUN_APP_HEAP_SIZE);
    if ((uint64_t)k_l0[slot] + L0_PTE_USER + 4u <= env->vm->ram_size)
        wr32(ram + k_l0[slot] + L0_PTE_USER, 0u);
    haeun_tlb_flush(env->vm);
}

static int extract_user(const uint8_t *image, size_t image_size, uint8_t *dst,
                        uint32_t dst_cap, uint32_t *out_len, uint32_t *entry) {
    uint16_t count;
    uint32_t table;
    uint32_t i;
    if (haeun_sec_validate_exec(image, image_size) != HAEUN_SEC_OK)
        return -1;
    if (image_size < 64u)
        return -1;
    *entry = rd32(image + 16u);
    if (*entry != HAEUN_APP_USER_VA)
        return -1;
    table = rd32(image + 20u);
    count = (uint16_t)(image[24] | (image[25] << 8));
    if (count == 0u || table + (uint32_t)count * 32u > image_size)
        return -1;
    for (i = 0u; i < count; ++i) {
        const uint8_t *raw = image + table + i * 32u;
        uint32_t flags = rd32(raw + 4u);
        uint32_t off = rd32(raw + 8u);
        uint32_t va = rd32(raw + 12u);
        uint32_t fsz = rd32(raw + 16u);
        uint32_t msz = rd32(raw + 20u);
        if (va != HAEUN_APP_USER_VA || (flags & 4u) == 0u)
            continue;
        if (fsz > dst_cap || fsz > msz || msz > HAEUN_APP_CODE_MAX ||
            off + fsz > image_size)
            return -1;
        memset(dst, 0, dst_cap);
        memcpy(dst, image + off, fsz);
        *out_len = fsz;
        return 0;
    }
    return -1;
}

/**
 * Binds the application environment to a process-v1 guest VM and optional WM.
 */
haeun_app_result haeun_app_env_init(haeun_app_env *env, haeun_vm *vm,
                                    haeun_gfx_wm *wm) {
    if (env == NULL || vm == NULL)
        return HAEUN_APP_BAD_ARGUMENT;
    memset(env, 0, sizeof(*env));
    env->vm = vm;
    env->wm = wm;
    env->next_pid = 2u;
    return HAEUN_APP_OK;
}

/**
 * Registers a named HXF application image (validated, not copied).
 */
haeun_app_result haeun_app_register(haeun_app_env *env, const char *name,
                                    const uint8_t *image, size_t image_size) {
    haeun_app_entry *e;
    uint8_t tmp[HAEUN_APP_CODE_MAX];
    uint32_t len = 0u;
    uint32_t entry = 0u;
    int i;
    if (env == NULL || name == NULL || image == NULL || name[0] == '\0')
        return HAEUN_APP_BAD_ARGUMENT;
    if (strlen(name) >= HAEUN_APP_NAME_MAX)
        return HAEUN_APP_INVALID;
    if (env->app_count >= HAEUN_APP_MAX)
        return HAEUN_APP_NOSPC;
    for (i = 0; i < env->app_count; ++i) {
        if (env->apps[i].used && strcmp(env->apps[i].name, name) == 0)
            return HAEUN_APP_INVALID;
    }
    if (extract_user(image, image_size, tmp, sizeof(tmp), &len, &entry) != 0) {
        app_log("register: invalid HXF");
        return HAEUN_APP_INVALID;
    }
    e = &env->apps[env->app_count];
    memset(e, 0, sizeof(*e));
    e->used = 1;
    strncpy(e->name, name, HAEUN_APP_NAME_MAX - 1u);
    e->image = image;
    e->image_size = image_size;
    env->app_count++;
    (void)len;
    return HAEUN_APP_OK;
}

/** Returns the number of registered applications. */
int haeun_app_count(const haeun_app_env *env) {
    return env != NULL ? env->app_count : 0;
}

/**
 * Copies the application name at index into out (NUL-terminated).
 */
haeun_app_result haeun_app_name(const haeun_app_env *env, int index, char *out,
                                size_t out_cap) {
    if (env == NULL || out == NULL || out_cap == 0u || index < 0 ||
        index >= env->app_count || !env->apps[index].used)
        return HAEUN_APP_NOENT;
    strncpy(out, env->apps[index].name, out_cap - 1u);
    out[out_cap - 1u] = '\0';
    return HAEUN_APP_OK;
}

/** Destroys every WM window owned by pid (pid must be nonzero). */
haeun_app_result haeun_app_destroy_windows(haeun_gfx_wm *wm, uint32_t pid) {
    int i;
    if (wm == NULL || pid == 0u)
        return HAEUN_APP_BAD_ARGUMENT;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (wm->windows[i].used && wm->windows[i].owner_pid == (int)pid)
            (void)haeun_gfx_wm_destroy(wm, i);
    }
    return HAEUN_APP_OK;
}

/**
 * Marks a slot free when its pid has exited; destroys windows and scrubs AS.
 * SPAWN: clears runs[] and scrubs slots 1–2.
 * FORK-only: scrubs slots 1–2 whose PCB.pid matches while user PTE is live
 * (idempotent — second reap after PTE clear returns NOENT; ZOMBIE PCB.pid
 * may remain until WAIT/SPAWN reclaim).
 * Slot 0 (init/desktop) is never scrubbed.
 */
haeun_app_result haeun_app_reap(haeun_app_env *env, uint32_t pid) {
    int i;
    int had_run = 0;
    int run_cleared = 0;
    int fork_only_scrub = 0;
    if (env == NULL || pid == 0u)
        return HAEUN_APP_BAD_ARGUMENT;
    if (env->wm != NULL)
        (void)haeun_app_destroy_windows(env->wm, pid);
    for (i = 0; i < HAEUN_APP_SLOTS; ++i) {
        if (env->runs[i].used && env->runs[i].pid == pid)
            had_run = 1;
    }
    if (env->vm != NULL && env->vm->ram != NULL) {
        for (i = 1; i < HAEUN_APP_SLOTS; ++i) {
            uint8_t *pcb =
                env->vm->ram + PCB_BASE + (uint32_t)i * PCB_STRIDE;
            if (rd32(pcb + 4u) != pid)
                continue;
            if (had_run) {
                scrub_slot(env, i);
            } else if (haeun_app_slot_user_pte(env, i) != 0u) {
                /* First FORK-only reap; PTE==0 means already scrubbed. */
                scrub_slot(env, i);
                fork_only_scrub = 1;
            }
        }
    }
    for (i = 0; i < HAEUN_APP_SLOTS; ++i) {
        if (env->runs[i].used && env->runs[i].pid == pid) {
            if (i >= 1)
                scrub_slot(env, i);
            env->runs[i].used = 0;
            env->runs[i].pid = 0u;
            env->runs[i].app_index = -1;
            env->runs[i].slot = 0;
            run_cleared = 1;
        }
    }
    if (run_cleared || fork_only_scrub)
        return HAEUN_APP_OK;
    return HAEUN_APP_NOENT;
}

/**
 * Validates, loads into a free AS slot, and starts a READY process.
 */
haeun_app_result haeun_app_spawn(haeun_app_env *env, int index, uint32_t *pid) {
    int slot;
    uint8_t *ram;
    uint8_t code[HAEUN_APP_CODE_MAX];
    uint32_t clen = 0u;
    uint32_t entry = 0u;
    uint32_t new_pid;
    uint32_t ppid;
    uint8_t *pcb;
    if (env == NULL || env->vm == NULL || pid == NULL || index < 0 ||
        index >= env->app_count || !env->apps[index].used)
        return HAEUN_APP_BAD_ARGUMENT;
    if ((env->apps[index].flags & HAEUN_APP_FLAG_SINGLE) != 0u) {
        for (slot = 1; slot < HAEUN_APP_SLOTS; ++slot) {
            if (env->runs[slot].used && env->runs[slot].app_index == index)
                return HAEUN_APP_BUSY;
        }
    }
    if (extract_user(env->apps[index].image, env->apps[index].image_size, code,
                     sizeof(code), &clen, &entry) != 0)
        return HAEUN_APP_INVALID;
    ram = env->vm->ram;
    /* Desktop may let apps EXIT without WAIT; reclaim orphan zombies so the
     * next spawn is not stuck on a ZOMBIE PCB with a cleared run slot. */
    for (slot = 1; slot < HAEUN_APP_SLOTS; ++slot) {
        uint8_t *opcb = ram + PCB_BASE + (uint32_t)slot * PCB_STRIDE;
        if (!env->runs[slot].used && rd32(opcb) == STATE_ZOMBIE)
            memset(opcb, 0, PCB_STRIDE);
    }
    for (slot = 1; slot < HAEUN_APP_SLOTS; ++slot) {
        if (!env->runs[slot].used &&
            rd32(ram + PCB_BASE + (uint32_t)slot * PCB_STRIDE) == STATE_FREE)
            break;
    }
    if (slot >= HAEUN_APP_SLOTS) {
        (void)fprintf(stderr, "app-env: spawn NOSPC index=%d\n", index);
        return HAEUN_APP_NOSPC;
    }
    if ((uint64_t)k_code_pa[slot] + HAEUN_APP_CODE_MAX > env->vm->ram_size)
        return HAEUN_APP_BAD_ARGUMENT;
    /* Mutate guest state only after validation; wipe prior code/stack first. */
    memset(ram + k_code_pa[slot], 0, HAEUN_APP_CODE_MAX);
    if ((uint64_t)k_ustack_page[slot] + 4096u <= env->vm->ram_size)
        memset(ram + k_ustack_page[slot], 0, 4096u);
    if ((uint64_t)k_heap_pa[slot] + HAEUN_APP_HEAP_SIZE <= env->vm->ram_size)
        memset(ram + k_heap_pa[slot], 0, HAEUN_APP_HEAP_SIZE);
    memcpy(ram + k_code_pa[slot], code, clen);
    wr32(ram + k_l0[slot] + L0_PTE_USER, k_code_pa[slot] | PTE_FLAGS);
    wr32(ram + k_l0[slot] + L0_PTE_HEAP, k_heap_pa[slot] | PTE_HEAP_FLAGS);
    haeun_tlb_flush(env->vm);
    new_pid = rd32(ram + PCB_NEXT_PID);
    if (new_pid < 2u)
        new_pid = env->next_pid;
    wr32(ram + PCB_NEXT_PID, new_pid + 1u);
    env->next_pid = new_pid + 1u;
    ppid = haeun_guest_pcb_current_pid(env->vm);
    pcb = ram + PCB_BASE + (uint32_t)slot * PCB_STRIDE;
    memset(pcb, 0, PCB_STRIDE);
    wr32(pcb + 0u, STATE_READY);
    wr32(pcb + 4u, new_pid);
    wr32(pcb + 8u, ppid);
    wr32(pcb + 12u, HAEUN_APP_USER_VA);
    wr32(pcb + 28u, k_ptbr[slot]);
    wr32(pcb + 32u, k_kstack[slot]);
    wr32(pcb + 36u, k_ustack_top[slot]);
    wr32(pcb + 40u, k_ustack_base[slot]);
    wr32(pcb + 44u, 1u); /* console fd0 */
    wr32(pcb + 116u, k_ustack_top[slot]); /* saved r13 */
    /* R1.2 ready_since + R3.4 ready_seq allocation (host mirror of take). */
    wr32(pcb + 160u, (uint32_t)env->vm->timer_count);
    {
        uint32_t glo = rd32(ram + 0xA46Cu);
        uint32_t ghi = rd32(ram + 0xA470u);
        glo += 1u;
        if (glo == 0u) {
            ghi += 1u;
            if (ghi == 0u)
                wr32(ram + 0xA478u, 1u);
        }
        wr32(ram + 0xA46Cu, glo);
        wr32(ram + 0xA470u, ghi);
        wr32(pcb + 172u, glo);
        wr32(pcb + 176u, ghi);
    }
    env->runs[slot].used = 1;
    env->runs[slot].app_index = index;
    env->runs[slot].pid = new_pid;
    env->runs[slot].slot = slot;
    *pid = new_pid;
    (void)printf("app-env: spawn %s pid=%u slot=%d\n", env->apps[index].name,
                 new_pid, slot);
    (void)entry;
    return HAEUN_APP_OK;
}
