/**
 * M25.2: scrub on SPAWN/reap; slot0 untouched; no stale tag after reuse.
 */
#include "haeun/app_env.h"
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

static void prep(uint8_t *ram, size_t n) {
    int i;
    memset(ram, 0, n);
    for (i = 0; i < 4; ++i)
        wr32(ram + PCB_BASE + (uint32_t)i * PCB_STRIDE, 0u);
    wr32(ram + PCB_NEXT, 2u);
    wr32(ram + 0xA400u, 0u);
    wr32(ram + PCB_BASE + 4u, 1u);
    wr32(ram + PCB_BASE + 0u, 2u);
}

static void make_tiny_hxf(uint8_t *out, size_t *out_len, uint8_t tag) {
    uint8_t code[8] = {0, 0, 0, 0, 0x00, 0x00, 0x00, 0x04};
    size_t total = 96u + sizeof(code);
    code[0] = tag;
    memset(out, 0, total);
    out[0] = 'H';
    out[1] = 'X';
    out[2] = 'F';
    out[3] = '1';
    out[4] = 64;
    out[6] = 1;
    out[8] = 4;
    wr32(out + 16, 0x00108000u);
    out[20] = 64;
    out[24] = 1;
    out[26] = 32;
    wr32(out + 28, (uint32_t)total);
    wr32(out + 64, 1u);
    wr32(out + 68, 7u);
    wr32(out + 72, 96u);
    wr32(out + 76, 0x00108000u);
    wr32(out + 80, 8u);
    wr32(out + 84, 8u);
    wr32(out + 88, 4u);
    memcpy(out + 96, code, 8);
    *out_len = total;
}

static int test_scrub_and_slot0(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img_a[256], img_b[256];
    size_t la = 0u, lb = 0u;
    uint32_t p = 0u, q = 0u;
    uint32_t pa1, pa0;
    uint32_t pte;
    prep(ram, sizeof(ram));
    /* Poison slot0 code page sentinel — must survive reap of other pids. */
    memset(ram + 0x00108000u, 0x5Cu, 64u);
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img_a, &la, 0x11u);
    make_tiny_hxf(img_b, &lb, 0x22u);
    CHECK(haeun_app_register(&env, "a", img_a, la) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "b", img_b, lb) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p) == HAEUN_APP_OK);
    pa1 = haeun_app_slot_code_pa(1);
    CHECK(ram[pa1] == 0x11u);
    pte = haeun_app_slot_user_pte(&env, 1);
    CHECK((pte & 1u) != 0u);
    CHECK(haeun_app_reap(&env, p) == HAEUN_APP_OK);
    CHECK(ram[pa1] == 0u);
    CHECK(haeun_app_slot_user_pte(&env, 1) == 0u);
    pa0 = haeun_app_slot_code_pa(0);
    CHECK(ram[pa0] == 0x5Cu); /* slot0 not scrubbed */
    wr32(ram + PCB_BASE + PCB_STRIDE, 0u);
    CHECK(haeun_app_spawn(&env, 1, &q) == HAEUN_APP_OK);
    CHECK(ram[haeun_app_slot_code_pa(1)] == 0x22u); /* no 0x11 leak */
    CHECK(haeun_app_reap(&env, 1u) == HAEUN_APP_NOENT); /* wrong pid */
    CHECK(ram[pa0] == 0x5Cu);
    return 0;
}

/** FORK-only PCB pid scrub via reap without runs[]. */
static int test_fork_pid_scrub(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint32_t pa;
    prep(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    pa = haeun_app_slot_code_pa(1);
    memset(ram + pa, 0xABu, 32u);
    wr32(ram + 0x00007000u + 0x420u, pa | 0x1bu);
    wr32(ram + PCB_BASE + PCB_STRIDE + 0u, 5u); /* ZOMBIE */
    wr32(ram + PCB_BASE + PCB_STRIDE + 4u, 99u);
    CHECK(haeun_app_reap(&env, 99u) == HAEUN_APP_OK);
    CHECK(ram[pa] == 0u);
    CHECK(haeun_app_slot_user_pte(&env, 1) == 0u);
    CHECK(haeun_app_reap(&env, 99u) == HAEUN_APP_NOENT);
    return 0;
}

int main(void) {
    if (test_scrub_and_slot0() != 0)
        return 1;
    if (test_fork_pid_scrub() != 0)
        return 1;
    puts("m25_2_memory_scrub_tests ok");
    return 0;
}
