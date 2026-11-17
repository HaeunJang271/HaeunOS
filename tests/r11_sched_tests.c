/**
 * R1.1 process-v1 scheduler context regression.
 * Usage: r11_sched_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define CURR 0x0000A400u
#define STRIDE 0x100u

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

static int sample(const haeun_vm *vm, const uint8_t *ram) {
    uint32_t curr;
    uint32_t state;
    if (vm->privilege != HAEUN_PRIV_USER)
        return 0;
    curr = rd32(ram, CURR);
    if (curr > 2u) {
        fprintf(stderr, "user curr=%u pc=0x%08x\n", curr, vm->pc);
        return 1;
    }
    state = rd32(ram, PCB + curr * STRIDE);
    if (state != 2u) {
        fprintf(stderr, "user state=%u curr=%u pc=0x%08x\n", state, curr, vm->pc);
        return 1;
    }
    return 0;
}

/**
 * Loads one HXF and steps until halt or cap. Rejects a user-mode blocked PCB.
 */
static int load_run(const char *path, haeun_vm *vm, uint8_t *ram, uint64_t cap,
                    haeun_vm_result *st, int poke_slot3) {
    FILE *f;
    uint8_t *image;
    long n;
    size_t got;
    uint32_t entry = 0u;
    uint32_t isa = 0u;
    uint64_t i;
    int poked = 0;
    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "open failed %s\n", path);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "size failed %s\n", path);
        (void)fclose(f);
        return 1;
    }
    image = (uint8_t *)malloc((size_t)n);
    if (image == NULL) {
        (void)fclose(f);
        return 1;
    }
    got = fread(image, 1u, (size_t)n, f);
    (void)fclose(f);
    if (got != (size_t)n) {
        free(image);
        fprintf(stderr, "read failed %s\n", path);
        return 1;
    }
    memset(ram, 0, RAM_SIZE);
    if (haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) != HAEUN_HXF_OK) {
        free(image);
        fprintf(stderr, "hxf load failed %s\n", path);
        return 1;
    }
    free(image);
    if (haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) != HAEUN_VM_OK) {
        fprintf(stderr, "vm init failed %s\n", path);
        return 1;
    }
    *st = HAEUN_VM_OK;
    for (i = 0; i < cap; ++i) {
        if (!poked && poke_slot3 && vm->privilege == HAEUN_PRIV_USER) {
            ram[PCB + 3u * STRIDE] = 1u;
            ram[PCB + 3u * STRIDE + 12u] = 0x00u;
            ram[PCB + 3u * STRIDE + 13u] = 0xD0u;
            ram[PCB + 3u * STRIDE + 14u] = 0xBAu;
            ram[PCB + 3u * STRIDE + 15u] = 0x00u;
            poked = 1;
        }
        *st = haeun_vm_step(vm);
        if (sample(vm, ram) != 0)
            return 1;
        if (vm->privilege == HAEUN_PRIV_USER && vm->csr_epc == 0x00BAD000u) {
            fprintf(stderr, "sentinel epc resumed\n");
            return 1;
        }
        if (*st == HAEUN_VM_HALTED)
            return 0;
        if (*st != HAEUN_VM_OK) {
            fprintf(stderr, "step status %d pc=0x%08x\n", (int)*st, vm->pc);
            return 1;
        }
    }
    return 0;
}
/** Yield loop then EXIT 11. User SP and ptbr stay on PCB0. */
static int test_yield(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint32_t sp;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN yield\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 11u);
    CHECK(rd32(ram, PCB) == 5u);
    CHECK(rd32(ram, CURR) == 0u);
    CHECK(rd32(ram, PCB + 28u) == 0x00002001u);
    CHECK(vm.csr_ptbr == 0x00002001u);
    sp = rd32(ram, PCB + 116u);
    CHECK(sp >= 0x00113000u && sp <= 0x00114000u);
    CHECK(rd32(ram, PCB + 12u) >= 0x00108000u && rd32(ram, PCB + 12u) < 0x00109000u);
    CHECK(vm.csr_epc == rd32(ram, PCB + 12u));
    free(ram);
    printf("yield ok sp=0x%08x epc=0x%08x\n", sp, vm.csr_epc);
    return 0;
}
/** Repeated SLEEP must not resume until the PCB is RUNNING again. */
static int test_sleep(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_sleep.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN sleep\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 33u);
    CHECK(rd32(ram, PCB) == 5u);
    free(ram);
    printf("sleep ok\n");
    return 0;
}
/** Parent reaps the child exit code. */
static int test_wait(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_wait.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN wait\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 4u);
    CHECK(rd32(ram, PCB) == 5u);
    CHECK(rd32(ram, PCB + STRIDE) == 0u);
    free(ram);
    printf("wait ok\n");
    return 0;
}
/** EXIT leaves a yielding sibling running. */
static int test_sibling(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    int saw_child = 0;
    uint64_t i;
    snprintf(path, sizeof(path), "%s/r11_sibling.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN sibling\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_OK);
    CHECK(rd32(ram, PCB) == 5u);
    CHECK(rd32(ram, PCB + 16u) == 6u);
    for (i = 0; i < 5000u; ++i) {
        st = haeun_vm_step(&vm);
        CHECK(sample(&vm, ram) == 0);
        CHECK(st == HAEUN_VM_OK);
        if (vm.privilege == HAEUN_PRIV_USER && rd32(ram, CURR) == 1u)
            saw_child = 1;
    }
    CHECK(saw_child);
    CHECK(rd32(ram, PCB + STRIDE) == 2u || rd32(ram, PCB + STRIDE) == 1u);
    free(ram);
    printf("sibling ok\n");
    return 0;
}

/** Faulted child is reaped; parent exits 0. */
static int test_fault(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_fault.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN fault\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 0u);
    CHECK(rd32(ram, PCB + STRIDE) == 0u);
    free(ram);
    printf("fault ok\n");
    return 0;
}

/** WAIT with nobody to wake halts as deadlock r1=4 and stays out of user mode. */
static int test_dead(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_dead.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN deadlock\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 0);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 4u);
    CHECK(rd32(ram, PCB) == 4u);
    free(ram);
    printf("deadlock ok\n");
    return 0;
}

/** A READY PCB3 with a sentinel epc is never dispatched. */
static int test_slot3(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    int rc;
    snprintf(path, sizeof(path), "%s/r11_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN slot3\n");
    rc = load_run(path, &vm, ram, 200000u, &st, 1);
    CHECK(rc == 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(rd32(ram, CURR) != 3u);
    CHECK(rd32(ram, PCB + 3u * STRIDE) == 1u);
    CHECK(rd32(ram, PCB + 3u * STRIDE + 12u) == 0x00BAD000u);
    free(ram);
    printf("slot3 ok\n");
    return 0;
}

/**
 * Runs the R1.1 scheduler images from the directory given on the command line.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r11_sched_tests <build-dir>\n");
        return 2;
    }
    if (test_yield(argv[1]) || test_sleep(argv[1]) || test_wait(argv[1]) ||
        test_sibling(argv[1]) || test_fault(argv[1]) || test_dead(argv[1]) ||
        test_slot3(argv[1]))
        return 1;
    printf("r11 sched tests passed\n");
    return 0;
}
