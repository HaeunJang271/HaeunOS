/**
 * R3.3 guest shadow planner after halt (one VM lifetime).
 * Usage: r33_planner_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/planner.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define POL 0xA474u

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

static int load_halt(const char *path, haeun_vm *vm, uint8_t **ram_out) {
    FILE *f;
    uint8_t *image;
    uint8_t *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 0 &&
          fseek(f, 0, SEEK_SET) == 0);
    image = (uint8_t *)malloc((size_t)n);
    CHECK(image != NULL &&
          (got = fread(image, 1u, (size_t)n, f)) == (size_t)n);
    (void)fclose(f);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) ==
          HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    for (i = 0; i < 400000u; ++i) {
        st = haeun_vm_step(vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    *ram_out = ram;
    return 0;
}

static int test_shadow_rr(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_plan_session sess;
    haeun_plan plan;
    uint32_t before;
    printf("RUN guest shadow rr\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    if (load_halt(path, &vm, &ram))
        return 1;
    before = rd32(ram, POL);
    CHECK(before == 0u);
    haeun_plan_session_init(&sess);
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    CHECK(plan.shadow_mode);
    CHECK(!plan.txn_committed);
    CHECK(rd32(ram, POL) == before);
    haeun_plan_fprint(stdout, &plan);
    free(ram);
    printf("guest shadow rr ok\n");
    return 0;
}

static int test_shadow_fcfs(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_plan_session sess;
    haeun_plan plan;
    uint32_t before;
    printf("RUN guest shadow fcfs\n");
    snprintf(path, sizeof(path), "%s/fcfs/r12_yield.hxf", build);
    if (load_halt(path, &vm, &ram))
        return 1;
    before = rd32(ram, POL);
    CHECK(before == 1u);
    haeun_plan_session_init(&sess);
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    CHECK(plan.shadow_mode);
    CHECK(!plan.txn_committed);
    CHECK(rd32(ram, POL) == before);
    haeun_plan_fprint_json(stdout, &plan);
    fputc('\n', stdout);
    free(ram);
    printf("guest shadow fcfs ok\n");
    return 0;
}

/**
 * Entry for R3.3 guest planner tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r33_planner_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_shadow_rr(argv[1]) || test_shadow_fcfs(argv[1]))
        return 1;
    printf("r33 planner guest tests passed\n");
    return 0;
}
