/**
 * R3.4 active runtime reconfiguration while eligible processes exist.
 * Usage: r34_active_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/policy_verify.h"
#include "haeun/pval_validate.h"
#include "haeun/reconfig.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define POL 0xA474u
#define PCB 0xA000u
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

static unsigned count_eligible(const uint8_t *ram) {
    unsigned n = 0u, i;
    for (i = 0; i < 3u; ++i) {
        uint32_t st = rd32(ram, PCB + i * STRIDE);
        if (st == 1u || st == 2u)
            n++;
    }
    return n;
}

static int eligible_seqs_ok(const uint8_t *ram) {
    unsigned i;
    for (i = 0; i < 3u; ++i) {
        uint32_t st = rd32(ram, PCB + i * STRIDE);
        uint32_t lo, hi;
        if (st != 1u && st != 2u)
            continue;
        lo = rd32(ram, PCB + i * STRIDE + 172u);
        hi = rd32(ram, PCB + i * STRIDE + 176u);
        if (lo == 0u && hi == 0u)
            return 0;
    }
    return 1;
}

static int load_vm(const char *path, haeun_vm *vm, uint8_t **ram_out) {
    FILE *f;
    uint8_t *image, *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
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
    *ram_out = ram;
    return 0;
}

static int try_switch(haeun_vm *vm, uint32_t src, uint32_t dst,
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

static int run_active_switch(const char *path, uint32_t boot_pol, uint32_t target,
                             const char *label) {
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_sched_trace tr;
    haeun_recfg_txn txn;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    int switched = 0;
    unsigned post_pol_events = 0u;
    unsigned elig_at_switch = 0u;
    printf("RUN %s\n", label);
    if (load_vm(path, &vm, &ram))
        return 1;
    /* 0xA474 is 0 until sched_pol_init; do not require boot_pol at load. */
    haeun_sched_trace_reset(&tr);
    for (i = 0; i < 250000u; ++i) {
        st = haeun_vm_step(&vm);
        haeun_sched_trace_observe(&tr, &vm);
        if (!switched && st == HAEUN_VM_OK) {
            unsigned elig = count_eligible(ram);
            if (elig >= 2u && eligible_seqs_ok(ram) && i > 2000u &&
                rd32(ram, POL) == boot_pol) {
                elig_at_switch = elig;
                CHECK(try_switch(&vm, boot_pol, target, &txn) ==
                      (int)HAEUN_RECFG_OK);
                CHECK(txn.state == HAEUN_RECFG_ST_COMMITTED);
                CHECK(rd32(ram, POL) == target);
                switched = 1;
                printf("  switched at step=%llu eligible=%u\n",
                       (unsigned long long)i, elig);
            }
        }
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(switched);
    CHECK(elig_at_switch >= 2u);
    CHECK(rd32(ram, POL) == target);
    {
        unsigned e;
        for (e = 0; e < tr.count; ++e) {
            if (tr.events[e].policy_id == target)
                post_pol_events++;
        }
    }
    CHECK(post_pol_events >= 1u);
    printf("  trace_events=%u post_target_policy_events=%u\n", tr.count,
           post_pol_events);
    free(ram);
    printf("%s ok\n", label);
    return 0;
}

static int test_bidirectional(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    int phase = 0;
    printf("RUN active bidirectional\n");
    snprintf(path, sizeof(path), "%s/r12_pair.hxf", build);
    if (load_vm(path, &vm, &ram))
        return 1;
    for (i = 0; i < 300000u; ++i) {
        st = haeun_vm_step(&vm);
        if (st != HAEUN_VM_OK && st != HAEUN_VM_HALTED)
            return 1;
        if (phase == 0 && st == HAEUN_VM_OK && count_eligible(ram) >= 2u &&
            eligible_seqs_ok(ram) && i > 2000u && rd32(ram, POL) == 0u) {
            CHECK(try_switch(&vm, 0u, 1u, &txn) == (int)HAEUN_RECFG_OK);
            CHECK(rd32(ram, POL) == 1u);
            phase = 1;
            printf("  RR->FIFO at %llu\n", (unsigned long long)i);
        } else if (phase == 1 && st == HAEUN_VM_OK && count_eligible(ram) >= 2u &&
                   eligible_seqs_ok(ram) && i > 8000u &&
                   rd32(ram, POL) == 1u) {
            CHECK(try_switch(&vm, 1u, 0u, &txn) == (int)HAEUN_RECFG_OK);
            CHECK(rd32(ram, POL) == 0u);
            phase = 2;
            printf("  FIFO->RR at %llu\n", (unsigned long long)i);
        }
        if (st == HAEUN_VM_HALTED)
            break;
    }
    CHECK(phase == 2);
    free(ram);
    printf("active bidirectional ok\n");
    return 0;
}

/**
 * Entry for R3.4 active guest transition tests.
 */
int main(int argc, char **argv) {
    char path[512];
    if (argc != 2) {
        fprintf(stderr, "usage: r34_active_guest_tests <build-dir>\n");
        return 2;
    }
    snprintf(path, sizeof(path), "%s/r12_pair.hxf", argv[1]);
    if (run_active_switch(path, 0u, 1u, "active rr->fifo"))
        return 1;
    snprintf(path, sizeof(path), "%s/fcfs/r12_pair.hxf", argv[1]);
    if (run_active_switch(path, 1u, 0u, "active fifo->rr"))
        return 1;
    if (test_bidirectional(argv[1]))
        return 1;
    printf("r34 active guest tests passed\n");
    return 0;
}
