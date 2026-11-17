/**
 * R2.0 policy abstraction: RR equivalence, invalid ID, read-only policy poke.
 * Usage: r20_policy_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define STRIDE 0x100u
#define CURR 0x0000A400u
#define POL 0x0000A474u
#define T_CTX 0x0000A468u
#define T_IRQ 0x0000A464u

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static uint32_t rd32(const uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off] | ((uint32_t)ram[off + 1u] << 8) |
           ((uint32_t)ram[off + 2u] << 16) | ((uint32_t)ram[off + 3u] << 24);
}

static void wr32(uint8_t *ram, uint32_t off, uint32_t v) {
    ram[off] = (uint8_t)(v & 0xffu);
    ram[off + 1u] = (uint8_t)((v >> 8) & 0xffu);
    ram[off + 2u] = (uint8_t)((v >> 16) & 0xffu);
    ram[off + 3u] = (uint8_t)((v >> 24) & 0xffu);
}

static int open_image(const char *path, haeun_vm *vm, uint8_t *ram) {
    FILE *f;
    uint8_t *image;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    f = fopen(path, "rb");
    if (f == NULL)
        return 1;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return 1;
    }
    image = (uint8_t *)malloc((size_t)n);
    if (image == NULL || (got = fread(image, 1u, (size_t)n, f)) != (size_t)n) {
        (void)fclose(f);
        free(image);
        return 1;
    }
    (void)fclose(f);
    memset(ram, 0, RAM_SIZE);
    if (haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) != HAEUN_HXF_OK) {
        free(image);
        return 1;
    }
    free(image);
    return haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK ? 0 : 1;
}

static int step_halt(haeun_vm *vm, uint64_t cap, haeun_vm_result *st) {
    uint64_t i;
    *st = HAEUN_VM_OK;
    for (i = 0; i < cap; ++i) {
        *st = haeun_vm_step(vm);
        if (*st == HAEUN_VM_HALTED)
            return 0;
        if (*st != HAEUN_VM_OK)
            return 1;
    }
    return 0;
}

/** RR self-yield: matches R1.2 golden (dispatch>=40, ctx=0, exit 22). */
static int test_rr_yield(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN rr_yield\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 200000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 22u);
    CHECK(rd32(ram, POL) == 0u);
    CHECK(rd32(ram, PCB + 144u) >= 40u);
    CHECK(rd32(ram, T_CTX) == 0u);
    CHECK(rd32(ram, PCB) == 5u);
    CHECK(rd32(ram, CURR) == 0u);
    free(ram);
    printf("rr_yield ok\n");
    return 0;
}

/** Two processes: context_switches > 0 and below total dispatches (R1.2 pair). */
static int test_rr_pair(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t d0, d1, ctx;
    snprintf(path, sizeof(path), "%s/r12_pair.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN rr_pair\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 400000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    d0 = rd32(ram, PCB + 144u);
    d1 = rd32(ram, PCB + STRIDE + 144u);
    ctx = rd32(ram, T_CTX);
    CHECK(d0 >= 1u && d1 >= 1u);
    CHECK(ctx >= 1u);
    CHECK(ctx < d0 + d1);
    free(ram);
    printf("rr_pair ok ctx=%u\n", ctx);
    return 0;
}

/**
 * Invalid policy ID: after user mode, poke 0xA474=99.
 * Next schedule returns none → idle_dead halt r1=4; no context switch invent.
 */
static int test_bad_policy(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    int poked = 0;
    uint32_t ctx_before = 0u;
    uint8_t snap[0x400];
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN bad_policy\n");
    CHECK(open_image(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        if (!poked && vm.privilege == HAEUN_PRIV_USER) {
            CHECK(rd32(ram, POL) == 0u);
            ctx_before = rd32(ram, T_CTX);
            memcpy(snap, ram + PCB, sizeof(snap));
            wr32(ram, POL, 99u);
            poked = 1;
        }
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(poked);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 4u);
    CHECK(rd32(ram, POL) == 99u);
    CHECK(rd32(ram, T_CTX) == ctx_before);
    /* Policy must not have rewritten other slots' identity words blindly. */
    CHECK(rd32(ram, PCB + STRIDE + 4u) == rd32(snap, STRIDE + 4u));
    free(ram);
    printf("bad_policy ok\n");
    return 0;
}

/** Slot 3 READY poison never becomes curr (R1.1 invariant under new core). */
static int test_slot3(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    int poked = 0;
    snprintf(path, sizeof(path), "%s/r11_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN slot3\n");
    CHECK(open_image(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        if (!poked && vm.privilege == HAEUN_PRIV_USER) {
            wr32(ram, PCB + 3u * STRIDE, 1u);
            wr32(ram, PCB + 3u * STRIDE + 12u, 0x00BAD000u);
            poked = 1;
        }
        st = haeun_vm_step(&vm);
        if (vm.privilege == HAEUN_PRIV_USER)
            CHECK(rd32(ram, CURR) != 3u);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(poked);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(rd32(ram, PCB + 3u * STRIDE + 12u) == 0x00BAD000u);
    free(ram);
    printf("slot3 ok\n");
    return 0;
}

/** Timer wrap still produces user timer traps after abstraction. */
static int test_timer_wrap(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st;
    uint64_t i;
    uint32_t prev;
    unsigned traps = 0u;
    snprintf(path, sizeof(path), "%s/r11_spin.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN timer_wrap\n");
    CHECK(open_image(path, &vm, ram) == 0);
    vm.timer_count = UINT64_C(0xFFFFFFF0);
    vm.timer_compare = 0u;
    vm.timer_control = 0u;
    vm.irq_pending = 0u;
    for (i = 0; i < 8000u; ++i) {
        prev = vm.privilege;
        st = haeun_vm_step(&vm);
        if (prev == HAEUN_PRIV_USER && vm.privilege != HAEUN_PRIV_USER &&
            vm.csr_cause == 16u)
            traps++;
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(traps >= 3u);
    CHECK((vm.timer_compare >> 32) != 0u);
    CHECK(rd32(ram, T_IRQ) >= 3u);
    free(ram);
    printf("timer_wrap ok traps=%u\n", traps);
    return 0;
}

/**
 * Runs R2.0 policy regression images from the build directory.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r20_policy_tests <build-dir>\n");
        return 2;
    }
    if (test_rr_yield(argv[1]) || test_rr_pair(argv[1]) || test_bad_policy(argv[1]) ||
        test_slot3(argv[1]) || test_timer_wrap(argv[1]))
        return 1;
    printf("r20 policy tests passed\n");
    return 0;
}
