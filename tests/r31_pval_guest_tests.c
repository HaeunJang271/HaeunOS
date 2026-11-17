/**
 * R3.1 guest validation against RR/FCFS images after halt (no extra steps).
 * Usage: r31_pval_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/pval_validate.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)

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

static int run_image(const char *path, uint32_t expect_pol, uint32_t target) {
    FILE *f;
    uint8_t *image;
    uint8_t *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    haeun_pval_report rep;
    uint32_t pol_before;
    printf("RUN guest %s target=%u\n", path, target);
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
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    for (i = 0; i < 400000u; ++i) {
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(rd32(ram, 0xA474u) == expect_pol);
    pol_before = rd32(ram, 0xA474u);
    CHECK(haeun_pval_validate(&vm, target, &rep) == HAEUN_PVAL_OK);
    CHECK(rep.read_only == HAEUN_PVAL_PASS);
    CHECK(rd32(ram, 0xA474u) == pol_before);
    CHECK(rep.snap.policy_id == expect_pol);
    CHECK(rep.invariants.overall != HAEUN_PVAL_FAIL);
    free(ram);
    printf("guest ok overall=%d\n", (int)rep.overall);
    return 0;
}

/**
 * Entry for R3.1 guest validation tests.
 */
int main(int argc, char **argv) {
    char path[512];
    if (argc != 2) {
        fprintf(stderr, "usage: r31_pval_guest_tests <build-dir>\n");
        return 2;
    }
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", argv[1]);
    if (run_image(path, 0u, HAEUN_PVAL_POLICY_RR))
        return 1;
    snprintf(path, sizeof(path), "%s/fcfs/r12_yield.hxf", argv[1]);
    if (run_image(path, 1u, HAEUN_PVAL_POLICY_FCFS))
        return 1;
    printf("r31 pval guest tests passed\n");
    return 0;
}
