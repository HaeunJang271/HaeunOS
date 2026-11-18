/**
 * R3.7 guest/offline robustness suite (smoke preset).
 * Usage: r37_robust_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/robust.h"
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

static int test_pipeline(const char *build) {
    haeun_rb_state st;
    unsigned i;
    printf("RUN robustness_pipeline_smoke\n");
    haeun_rb_state_init(&st);
    snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build);
    CHECK(haeun_rb_preset_apply(&st.cfg, HAEUN_RB_PRESET_SMOKE) == HAEUN_RB_OK);
    CHECK(haeun_rb_run_experiments(&st) == HAEUN_RB_OK);
    CHECK(st.wl_count == (unsigned)HAEUN_WL_COUNT);
    CHECK(haeun_rb_run_sensitivity(&st) == HAEUN_RB_OK);
    CHECK(haeun_rb_workload_robustness(&st) == HAEUN_RB_OK);
    CHECK(haeun_rb_verify_recommendations(&st) == HAEUN_RB_OK);
    (void)haeun_rb_reconfig_stress(&st, NULL, 1); /* dry */
    haeun_rb_aggregate(&st);
    CHECK(st.overall != HAEUN_RB_OVERALL_FAIL);
    for (i = 0; i < st.wl_count; ++i) {
        CHECK(st.workloads[i].replay == HAEUN_RB_EXACT_MATCH ||
              st.workloads[i].replay == HAEUN_RB_METRIC_MATCH);
        printf("  %s verify=%d replay=%d sens=%d\n", st.workloads[i].workload_id,
               (int)st.workloads[i].verify, (int)st.workloads[i].replay,
               (int)st.workloads[i].sensitivity);
    }
    CHECK(haeun_rb_export(&st, "build/r37") == HAEUN_RB_OK);
    printf("robustness_pipeline_smoke ok overall=%d\n", (int)st.overall);
    return 0;
}

static int test_live_stress_optional(const char *build) {
    char path[512];
    FILE *f;
    uint8_t *image, *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    haeun_vm vm;
    haeun_rb_state st;
    uint64_t i;
    printf("RUN reconfig_stress_live\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 0 &&
          fseek(f, 0, SEEK_SET) == 0);
    image = (uint8_t *)malloc((size_t)n);
    CHECK(image && (got = fread(image, 1, (size_t)n, f)) == (size_t)n);
    (void)fclose(f);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram && haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) ==
                     HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    for (i = 0; i < 4000u; ++i)
        if (haeun_vm_step(&vm) != HAEUN_VM_OK)
            break;
    haeun_rb_state_init(&st);
    CHECK(haeun_rb_reconfig_stress(&st, &vm, 0) == HAEUN_RB_OK);
    /* Invalid target handled; some RR↔FIFO may UNKNOWN — not overall FAIL. */
    printf("  stress transitions=%u failures=%u\n", st.stress_transitions,
           st.stress_failures);
    free(ram);
    printf("reconfig_stress_live ok\n");
    return 0;
}

/**
 * Entry for R3.7 guest robustness tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r37_robust_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_pipeline(argv[1]) || test_live_stress_optional(argv[1]))
        return 1;
    printf("r37 robust guest tests passed\n");
    return 0;
}
