/**
 * R1.2 process-v1 telemetry regression. Reads guest RAM counters.
 * Usage: r12_telem_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define STRIDE 0x100u
#define T_IRQ 0x0000A464u
#define T_CTX 0x0000A468u

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

/** run_ticks, completion, timer_irqs vs guest cause-16 entries. */
static int test_run(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t prev, run_ticks, irqs;
    unsigned traps = 0u;
    uint64_t i;
    snprintf(path, sizeof(path), "%s/r12_run.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN run\n");
    CHECK(open_image(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        prev = vm.privilege;
        st = haeun_vm_step(&vm);
        if (prev == HAEUN_PRIV_USER && vm.privilege != HAEUN_PRIV_USER &&
            vm.csr_cause == 16u)
            traps++;
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 21u);
    run_ticks = rd32(ram, PCB + 128u);
    irqs = rd32(ram, T_IRQ);
    CHECK(run_ticks > 0u);
    CHECK(rd32(ram, PCB + 152u) == 1u);
    CHECK(irqs == traps);
    CHECK(rd32(ram, PCB + 144u) >= 1u);
    printf("run ok irqs=%u run_ticks=%u\n", irqs, run_ticks);
    free(ram);
    return 0;
}

/** Self yield: dispatch_count grows; context_switches stays 0. */
static int test_yield(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN yield\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 200000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 22u);
    CHECK(rd32(ram, PCB + 144u) >= 40u);
    CHECK(rd32(ram, T_CTX) == 0u);
    CHECK(rd32(ram, PCB + 128u) > 0u);
    free(ram);
    printf("yield ok\n");
    return 0;
}

/** Sleep accumulates sleep_ticks. */
static int test_sleep(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t sleep_ticks;
    snprintf(path, sizeof(path), "%s/r12_sleep.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN sleep\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 200000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 23u);
    sleep_ticks = rd32(ram, PCB + 136u);
    CHECK(sleep_ticks >= 64u);
    CHECK(rd32(ram, PCB + 152u) == 1u);
    printf("sleep ok sleep_ticks=%u\n", sleep_ticks);
    free(ram);
    return 0;
}

/** Child starts zeroed; parent blocked_ticks > 0; parent run not cleared. */
static int test_wait(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t parent_run = 0u, blocked;
    uint64_t i;
    int saw = 0;
    snprintf(path, sizeof(path), "%s/r12_wait.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN wait\n");
    CHECK(open_image(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        st = haeun_vm_step(&vm);
        if (!saw && rd32(ram, PCB + STRIDE) == 1u && rd32(ram, PCB + STRIDE + 4u) != 0u) {
            CHECK(rd32(ram, PCB + STRIDE + 128u) == 0u);
            CHECK(rd32(ram, PCB + STRIDE + 144u) == 0u);
            parent_run = rd32(ram, PCB + 128u);
            saw = 1;
        }
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(saw);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 5u);
    blocked = rd32(ram, PCB + 140u);
    CHECK(blocked > 0u);
    CHECK(rd32(ram, PCB + 128u) >= parent_run);
    CHECK(rd32(ram, PCB + 152u) == 1u);
    printf("wait ok blocked=%u\n", blocked);
    free(ram);
    return 0;
}

/** Alternating pair: context_switches > 0 and less than total dispatches. */
static int test_pair(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t d0, d1, ctx;
    snprintf(path, sizeof(path), "%s/r12_pair.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN pair\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 400000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    d0 = rd32(ram, PCB + 144u);
    d1 = rd32(ram, PCB + STRIDE + 144u);
    ctx = rd32(ram, T_CTX);
    CHECK(d0 >= 1u && d1 >= 1u);
    CHECK(ctx >= 1u);
    CHECK(ctx < d0 + d1);
    printf("pair ok ctx=%u d0=%u d1=%u\n", ctx, d0, d1);
    free(ram);
    return 0;
}

/** Fault child completion; parent completes once. */
static int test_fault(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    snprintf(path, sizeof(path), "%s/r12_fault.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN fault\n");
    CHECK(open_image(path, &vm, ram) == 0);
    CHECK(step_halt(&vm, 200000u, &st) == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(rd32(ram, PCB + 152u) == 1u);
    free(ram);
    printf("fault ok\n");
    return 0;
}

/**
 * Runs R1.2 telemetry images from the directory given on the command line.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r12_telem_tests <build-dir>\n");
        return 2;
    }
    if (test_run(argv[1]) || test_yield(argv[1]) || test_sleep(argv[1]) ||
        test_wait(argv[1]) || test_pair(argv[1]) || test_fault(argv[1]))
        return 1;
    printf("r12 telem tests passed\n");
    return 0;
}
