/**
 * M25.6 host: heap PA constants, SPAWN wipe, scrub zeros heap.
 */
#include "haeun/app_env.h"
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void make_tiny_hxf(uint8_t *img, size_t *len) {
    static const uint8_t code[8] = {0, 0, 0, 0, 0x04, 0x00, 0x00, 0x00};
    uint8_t seg[32];
    memset(img, 0, 256);
    memcpy(img, "HXF1", 4);
    wr32(img + 8, 1u);
    wr32(img + 16, HAEUN_APP_USER_VA);
    wr32(img + 20, 64u);
    img[24] = 1;
    img[26] = 32;
    memset(seg, 0, sizeof(seg));
    wr32(seg + 4, 4u);
    wr32(seg + 8, 96u);
    wr32(seg + 12, HAEUN_APP_USER_VA);
    wr32(seg + 16, 8u);
    wr32(seg + 20, 8u);
    memcpy(img + 64, seg, 32);
    memcpy(img + 96, code, 8);
    *len = 104;
}

static int test_pa_and_scrub(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len;
    uint32_t pid, h1, h2;
    int i;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_app_slot_heap_pa(0) == HAEUN_APP_SLOT_HEAP_PA0);
    CHECK(haeun_app_slot_heap_pa(1) == HAEUN_APP_SLOT_HEAP_PA1);
    CHECK(haeun_app_slot_heap_pa(2) == HAEUN_APP_SLOT_HEAP_PA2);
    CHECK(haeun_app_slot_heap_pa(0) != haeun_app_slot_code_pa(0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "h", img, len) == HAEUN_APP_OK);
    h1 = haeun_app_slot_heap_pa(1);
    memset(ram + h1, 0xABu, 64u);
    CHECK(haeun_app_spawn(&env, 0, &pid) == HAEUN_APP_OK);
    for (i = 0; i < 64; ++i)
        CHECK(ram[h1 + (uint32_t)i] == 0u);
    memset(ram + h1, 0xCDu, 32u);
    CHECK(haeun_app_reap(&env, pid) == HAEUN_APP_OK);
    for (i = 0; i < 32; ++i)
        CHECK(ram[h1 + (uint32_t)i] == 0u);
    h2 = haeun_app_slot_heap_pa(0);
    memset(ram + h2, 0x5Cu, 8u);
    CHECK(haeun_app_reap(&env, 1u) == HAEUN_APP_NOENT);
    CHECK(ram[h2] == 0x5Cu);
    return 0;
}

int main(void) {
    if (test_pa_and_scrub() != 0)
        return 1;
    puts("m25_6_heap_host_tests ok");
    return 0;
}
