/**
 * R3.5 guest: shadow safety + fixed RR/FIFO/Adaptive over all catalog workloads.
 * Usage: r35_feedback_guest_tests <build-dir>
 */
#include "haeun/feedback.h"
#include "haeun/feedback_experiment.h"
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

static int test_shadow_unchanged(const char *build) {
    char path[512];
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_fb_state st;
    haeun_plan_session sess;
    haeun_plan plan;
    haeun_fb_assessment a;
    uint32_t pol_before;
    uint64_t i;
    printf("RUN shadow_unchanged\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    if (load_vm(path, &vm, &ram))
        return 1;
    for (i = 0; i < 5000u; ++i) {
        if (haeun_vm_step(&vm) != HAEUN_VM_OK)
            break;
    }
    pol_before = rd32(ram, POL);
    haeun_fb_state_init(&st);
    haeun_plan_session_init(&sess);
    st.mode = HAEUN_FB_MODE_SHADOW;
    CHECK(haeun_fb_cycle(&st, &sess, &vm, 0, &plan, &a) == HAEUN_FB_OK);
    CHECK(plan.txn_committed == 0);
    CHECK(rd32(ram, POL) == pol_before);
    free(ram);
    printf("shadow_unchanged ok pol=%u\n", pol_before);
    return 0;
}

static int test_all_workloads(const char *build) {
    static const haeun_eval_wl_kind kinds[] = {
        HAEUN_WL_CPU_BOUND,   HAEUN_WL_SHORT_JOB,    HAEUN_WL_MIXED,
        HAEUN_WL_BURSTY,      HAEUN_WL_BLOCK_SLEEP,  HAEUN_WL_BLOCK_WAIT};
    unsigned k;
    printf("RUN all_workloads_fixed_and_adaptive\n");
    for (k = 0; k < sizeof(kinds) / sizeof(kinds[0]); ++k) {
        haeun_fb_exp_result rr, ff, ad;
        CHECK(haeun_fb_exp_fixed(build, kinds[k], 0u, 1u, &rr) == HAEUN_FB_OK);
        CHECK(rr.mode == HAEUN_FB_EXP_FIXED_RR && rr.fail == 0u);
        CHECK(haeun_fb_exp_fixed(build, kinds[k], 1u, 1u, &ff) == HAEUN_FB_OK);
        CHECK(ff.mode == HAEUN_FB_EXP_FIXED_FIFO && ff.fail == 0u);
        CHECK(haeun_fb_exp_adaptive(build, kinds[k], 1u, 0, &ad) ==
              HAEUN_FB_OK);
        CHECK(ad.mode == HAEUN_FB_EXP_ADAPTIVE);
        CHECK(ad.transitions == 0u); /* SHADOW */
        printf("  wl=%s rr_ok=%u fifo_ok=%u adapt_ok=%u\n", rr.workload_id,
               rr.success, ff.success, ad.success);
    }
    printf("all_workloads_fixed_and_adaptive ok\n");
    return 0;
}

/**
 * Entry for R3.5 feedback guest tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r35_feedback_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_shadow_unchanged(argv[1]) || test_all_workloads(argv[1]))
        return 1;
    printf("r35 feedback guest tests passed\n");
    return 0;
}
