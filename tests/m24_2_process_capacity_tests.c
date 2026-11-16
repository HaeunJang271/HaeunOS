/**
 * M24.2 E4: NPROC=4, AS=3, SPAWN app capacity=2 are distinct limits.
 */
#include "haeun/app_env.h"
#include "haeun/guest_pcb.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum { PCB_BASE = 0xA000u, PCB_STRIDE = 0x100u, PCB_NEXT = 0xA404u };

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void prep_pcb_bank(uint8_t *ram, size_t n) {
    int i;
    memset(ram, 0, n);
    for (i = 0; i < 4; ++i)
        wr32(ram + PCB_BASE + (uint32_t)i * PCB_STRIDE, 0u);
    wr32(ram + PCB_NEXT, 2u);
    wr32(ram + 0xA400u, 0u);
    wr32(ram + PCB_BASE + 4u, 1u);
    wr32(ram + PCB_BASE + 0u, 2u);
}

static void make_tiny_hxf(uint8_t *out, size_t *out_len) {
    uint8_t code[8] = {0, 0, 0, 0, 0x00, 0x00, 0x00, 0x04};
    size_t total = 96u + sizeof(code);
    memset(out, 0, total);
    out[0] = 'H';
    out[1] = 'X';
    out[2] = 'F';
    out[3] = '1';
    out[4] = 64;
    out[6] = 1;
    out[8] = 4;
    out[16] = 0x00;
    out[17] = 0x80;
    out[18] = 0x10;
    out[19] = 0x00;
    out[20] = 64;
    out[24] = 1;
    out[26] = 32;
    out[28] = (uint8_t)total;
    out[64] = 1;
    out[68] = 7;
    out[72] = 96;
    out[76] = 0x00;
    out[77] = 0x80;
    out[78] = 0x10;
    out[79] = 0x00;
    out[80] = (uint8_t)sizeof(code);
    out[84] = (uint8_t)sizeof(code);
    out[88] = 4;
    memcpy(out + 96, code, sizeof(code));
    *out_len = total;
}

static int test_constants_and_spawn_cap(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u, p3 = 0u;
    int i;
    CHECK(HAEUN_GUEST_PCB_NPROC == 4);
    CHECK(HAEUN_GUEST_PCB_AS_SLOTS == 3);
    CHECK(HAEUN_APP_SLOTS == 3);
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "b", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 1, &p2) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_NOSPC);
    CHECK(env.runs[0].used == 0); /* slot 0 reserved for init/desktop */
    CHECK(env.runs[1].used && env.runs[2].used);
    CHECK(haeun_app_reap(&env, p1) == HAEUN_APP_OK);
    wr32(ram + PCB_BASE + 1u * PCB_STRIDE, 0u);
    CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_OK);
    CHECK(p3 > p2);
    /* PCB table has 4 slots; AS-usable FORK scan is 0..2 only (documented). */
    CHECK(rd32(ram + PCB_BASE + 3u * PCB_STRIDE) == 0u);
    /* Repeated create/destroy keeps SPAWN capacity recoverable. */
    (void)haeun_app_reap(&env, p2);
    (void)haeun_app_reap(&env, p3);
    for (i = 0; i < 8; ++i) {
        uint32_t px = 0u, py = 0u;
        wr32(ram + PCB_BASE + 1u * PCB_STRIDE, 0u);
        wr32(ram + PCB_BASE + 2u * PCB_STRIDE, 0u);
        env.runs[1].used = 0;
        env.runs[2].used = 0;
        CHECK(haeun_app_spawn(&env, 0, &px) == HAEUN_APP_OK);
        CHECK(haeun_app_spawn(&env, 1, &py) == HAEUN_APP_OK);
        CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_NOSPC);
        CHECK(haeun_app_reap(&env, px) == HAEUN_APP_OK);
        CHECK(haeun_app_reap(&env, py) == HAEUN_APP_OK);
    }
    return 0;
}

/** Orphan ZOMBIE PCB with cleared runs[] is reclaimed on next SPAWN. */
static int test_spawn_orphan_zombie_reclaim(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u;
    uint8_t *pcb1;
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_reap(&env, p1) == HAEUN_APP_OK);
    pcb1 = ram + PCB_BASE + PCB_STRIDE;
    wr32(pcb1, 5u); /* ZOMBIE left without WAIT */
    wr32(pcb1 + 4u, p1);
    CHECK(haeun_app_spawn(&env, 0, &p2) == HAEUN_APP_OK);
    CHECK(rd32(pcb1) == 1u); /* READY for new process */
    CHECK(rd32(pcb1 + 4u) == p2);
    return 0;
}

int main(void) {
    if (test_constants_and_spawn_cap() != 0)
        return 1;
    if (test_spawn_orphan_zombie_reclaim() != 0)
        return 1;
    puts("m24_2_process_capacity_tests ok");
    return 0;
}
