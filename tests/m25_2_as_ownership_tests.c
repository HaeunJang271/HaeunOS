/**
 * M25.2: AS slot ownership — exclusive runs[], scrub slot0 excluded, reuse.
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

static int test_exclusive_and_reuse(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u, p3 = 0u;
    haeun_app_run_info ri;
    uint32_t pa0, pa1;
    prep(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len, 0xAAu);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "b", img, len) == HAEUN_APP_OK);
    CHECK(env.runs[0].used == 0); /* slot0 reserved; SPAWN uses 1..2 */
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_run_at(&env, 1, &ri) == HAEUN_APP_OK);
    CHECK(ri.pid == p1);
    CHECK(haeun_app_spawn(&env, 1, &p2) == HAEUN_APP_OK);
    CHECK(p1 != p2);
    CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_NOSPC);
    pa0 = haeun_app_slot_code_pa(1);
    pa1 = haeun_app_slot_code_pa(2);
    CHECK(pa0 != pa1 && pa0 != 0u);
    CHECK(ram[pa0] == 0xAAu);
    CHECK(haeun_app_reap(&env, p1) == HAEUN_APP_OK);
    CHECK(haeun_app_run_at(&env, 1, &ri) == HAEUN_APP_NOENT);
    CHECK(ram[pa0] == 0u); /* scrubbed */
    wr32(ram + PCB_BASE + PCB_STRIDE, 0u);
    CHECK(haeun_app_spawn(&env, 0, &p3) == HAEUN_APP_OK);
    CHECK(p3 != p1);
    CHECK(haeun_app_slot_code_pa(0) == 0x00108000u);
    return 0;
}

int main(void) {
    if (test_exclusive_and_reuse() != 0)
        return 1;
    puts("m25_2_as_ownership_tests ok");
    return 0;
}
