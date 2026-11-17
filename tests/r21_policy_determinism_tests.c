/**
 * R2.1 host policy mirror, sched trace, core reject paths.
 * Usage: r21_policy_determinism_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/policy_verify.h"
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

static int open_hxf(const char *path, haeun_vm *vm, uint8_t *ram) {
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

/** Host mirror: determinism, eligibility, invalid ID, empty set. */
static int test_host_mirror(void) {
    haeun_policy_fixture fx;
    uint32_t a = 0u, b = 0u, curr;
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_RR;
    fx.state[0] = 1u;
    fx.state[1] = 3u;
    fx.state[2] = 1u;
    fx.state[3] = 1u;
    printf("RUN host_mirror\n");
    for (curr = 0u; curr < 3u; ++curr) {
        fx.curr = curr;
        CHECK(haeun_policy_select_host(&fx, &a) == 1);
        CHECK(haeun_policy_select_host(&fx, &b) == 1);
        CHECK(a == b);
    }
    fx.curr = 0u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1 && a == 2u);
    fx.curr = 2u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1 && a == 0u);
    fx.policy_id = 99u;
    CHECK(haeun_policy_select_host(&fx, &a) == 0);
    fx.policy_id = HAEUN_POLICY_RR;
    fx.state[0] = 0u;
    fx.state[1] = 4u;
    fx.state[2] = 5u;
    fx.state[3] = 2u;
    fx.curr = 0u;
    CHECK(haeun_policy_select_host(&fx, &a) == 0);
    fx.state[1] = 2u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1 && a == 1u);
    /* FREE/SLEEP/WAIT/ZOMBIE never win when a READY exists later in scan. */
    fx.state[0] = 0u;
    fx.state[1] = 3u;
    fx.state[2] = 1u;
    fx.curr = 0u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1 && a == 2u);
    printf("host_mirror ok\n");
    return 0;
}

/** Identical runs produce identical host sched traces (RR equivalence). */
static int test_trace_equiv(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_sched_trace tr1, tr2;
    haeun_vm_result st;
    uint64_t i;
    unsigned n;
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN trace_equiv\n");
    CHECK(open_hxf(path, &vm, ram) == 0);
    haeun_sched_trace_reset(&tr1);
    for (i = 0; i < 200000u; ++i) {
        st = haeun_vm_step(&vm);
        haeun_sched_trace_observe(&tr1, &vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(vm.r[1] == 22u);
    CHECK(tr1.count >= 1u);
    free(ram);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(open_hxf(path, &vm, ram) == 0);
    haeun_sched_trace_reset(&tr2);
    for (i = 0; i < 200000u; ++i) {
        st = haeun_vm_step(&vm);
        haeun_sched_trace_observe(&tr2, &vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(tr1.count == tr2.count);
    for (n = 0; n < tr1.count; ++n) {
        CHECK(tr1.events[n].selected_slot == tr2.events[n].selected_slot);
        CHECK(tr1.events[n].dispatch_count == tr2.events[n].dispatch_count);
        CHECK(tr1.events[n].context_switches == tr2.events[n].context_switches);
        CHECK(tr1.events[n].policy_id == 0u);
    }
    printf("trace sample: before=%u after=%u disp=%u ctx=%u\n",
           tr1.events[0].current_slot_before, tr1.events[0].current_slot_after,
           tr1.events[0].dispatch_count, tr1.events[0].context_switches);
    CHECK(rd32(ram, PCB + 144u) >= 40u);
    CHECK(rd32(ram, T_CTX) == 0u);
    free(ram);
    printf("trace_equiv ok events=%u\n", tr1.count);
    return 0;
}

/** Invalid policy and slot-3 poison rejected by core. */
static int test_core_rejects(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st;
    uint64_t i;
    int poked = 0;
    printf("RUN core_rejects\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(open_hxf(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        if (!poked && vm.privilege == HAEUN_PRIV_USER) {
            wr32(ram, POL, 7u);
            poked = 1;
        }
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(poked && vm.r[1] == 4u);
    free(ram);
    snprintf(path, sizeof(path), "%s/r11_yield.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(open_hxf(path, &vm, ram) == 0);
    poked = 0;
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
    CHECK(st == HAEUN_VM_HALTED);
    free(ram);
    printf("core_rejects ok\n");
    return 0;
}

/**
 * Runs R2.1 host-side determinism and rejection tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r21_policy_determinism_tests <build-dir>\n");
        return 2;
    }
    if (test_host_mirror() || test_trace_equiv(argv[1]) || test_core_rejects(argv[1]))
        return 1;
    printf("r21 policy determinism tests passed\n");
    return 0;
}
