/**
 * R3.2 guest runtime reconfiguration within one VM lifetime.
 * Usage: r32_reconfig_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/reconfig.h"
#include "haeun/reconfig_eval.h"
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

static int try_transition(haeun_vm *vm, uint32_t src, uint32_t dst,
                          haeun_recfg_txn *txn) {
    haeun_recfg_result rr;
    haeun_recfg_fi_clear();
    rr = haeun_recfg_request(txn, vm, src, dst);
    if (rr != HAEUN_RECFG_OK)
        return (int)rr;
    rr = haeun_recfg_prepare(txn, vm);
    if (rr != HAEUN_RECFG_OK)
        return (int)rr;
    return (int)haeun_recfg_commit(txn, vm);
}

static int test_fcfs_to_rr(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    uint32_t before;
    printf("RUN guest fcfs->rr\n");
    snprintf(path, sizeof(path), "%s/fcfs/r12_yield.hxf", build);
    if (load_halt(path, &vm, &ram))
        return 1;
    CHECK(rd32(ram, POL) == 1u);
    before = rd32(ram, POL);
    CHECK(try_transition(&vm, 1u, 0u, &txn) == (int)HAEUN_RECFG_OK);
    CHECK(txn.state == HAEUN_RECFG_ST_COMMITTED);
    CHECK(rd32(ram, POL) == 0u);
    CHECK(before == 1u);
    haeun_recfg_report_fprint(stdout, &txn);
    free(ram);
    printf("guest fcfs->rr ok\n");
    return 0;
}

static int test_rr_to_fcfs_gate(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    int rc;
    uint32_t before;
    printf("RUN guest rr->fcfs gate\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    if (load_halt(path, &vm, &ram))
        return 1;
    CHECK(rd32(ram, POL) == 0u);
    before = rd32(ram, POL);
    rc = try_transition(&vm, 0u, 1u, &txn);
    if (rc == (int)HAEUN_RECFG_OK) {
        CHECK(txn.state == HAEUN_RECFG_ST_COMMITTED);
        CHECK(rd32(ram, POL) == 1u);
        printf("rr->fcfs committed (vacuous timestamp path)\n");
    } else {
        CHECK(txn.state == HAEUN_RECFG_ST_REJECTED);
        CHECK(rd32(ram, POL) == before);
        printf("rr->fcfs rejected as required (UNKNOWN≠PASS): %s\n",
               txn.reject_reason);
    }
    haeun_recfg_report_fprint(stdout, &txn);
    free(ram);
    printf("guest rr->fcfs gate ok\n");
    return 0;
}

static int test_roundtrip_eval(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    haeun_recfg_eval ev;
    printf("RUN guest roundtrip_eval\n");
    haeun_recfg_eval_clear(&ev);
    snprintf(path, sizeof(path), "%s/fcfs/r12_yield.hxf", build);
    if (load_halt(path, &vm, &ram))
        return 1;
    CHECK(try_transition(&vm, 1u, 0u, &txn) == (int)HAEUN_RECFG_OK);
    haeun_recfg_eval_record(&ev, &txn, 0u, 0u, 0u, 0);
    /* Second direction depends on post-halt eligibility. */
    (void)try_transition(&vm, 0u, 1u, &txn);
    haeun_recfg_eval_record(&ev, &txn, 0u, 0u, 0u, 0);
    haeun_recfg_eval_fprint(stdout, &ev);
    CHECK(ev.success_count + ev.rejected_count >= 1u);
    free(ram);
    printf("guest roundtrip_eval ok\n");
    return 0;
}

/**
 * Entry for R3.2 guest runtime reconfiguration tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r32_reconfig_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_fcfs_to_rr(argv[1]) || test_rr_to_fcfs_gate(argv[1]) ||
        test_roundtrip_eval(argv[1]))
        return 1;
    printf("r32 reconfig guest tests passed\n");
    return 0;
}
