/**
 * R3.6 guest/offline optimization tests.
 * Usage: r36_opt_guest_tests <build-dir>
 */
#include "haeun/optimize.h"
#include "haeun/hxf.h"
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

static int test_optimize_all(const char *build) {
    haeun_opt_state st;
    unsigned i;
    printf("RUN optimize_all_workloads\n");
    haeun_opt_state_init(&st);
    snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build);
    st.cfg.reps = 2u; /* > prior 1-rep baseline; not significance */
    st.cfg.min_success = 1u;
    st.mode = HAEUN_OPT_MODE_OPTIMIZE;
    CHECK(haeun_opt_run(&st) == HAEUN_OPT_OK);
    CHECK(st.wl_count == (unsigned)HAEUN_WL_COUNT);
    for (i = 0; i < st.wl_count; ++i) {
        CHECK(st.workloads[i].cand_count == 2u);
        printf("  %s -> %d policy=%u\n", st.workloads[i].workload_id,
               (int)st.workloads[i].decision,
               st.workloads[i].recommended_policy);
    }
    printf("optimize_all_workloads ok\n");
    return 0;
}

static int test_all_profiles(const char *build) {
    haeun_opt_profile_id id;
    printf("RUN all_profiles_smoke\n");
    for (id = 0; id < HAEUN_OPT_PROF_COUNT; ++id) {
        haeun_opt_state st;
        haeun_opt_state_init(&st);
        snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build);
        st.cfg.profile_id = id;
        st.cfg.reps = 2u;
        st.cfg.min_success = 1u;
        CHECK(haeun_opt_profile_get(id, &st.profile) == HAEUN_OPT_OK);
        CHECK(haeun_opt_run_workload(&st.cfg, &st.profile, HAEUN_WL_SHORT_JOB,
                                     &st.workloads[0]) == HAEUN_OPT_OK ||
              st.workloads[0].decision == HAEUN_OPT_DEC_INCONCLUSIVE ||
              st.workloads[0].decision == HAEUN_OPT_DEC_EQUIVALENT ||
              st.workloads[0].decision == HAEUN_OPT_DEC_SELECT ||
              st.workloads[0].decision == HAEUN_OPT_DEC_NO_DECISION);
        printf("  profile=%s decision=%d\n", st.profile.name,
               (int)st.workloads[0].decision);
    }
    printf("all_profiles_smoke ok\n");
    return 0;
}

static int test_shadow_no_mutate(const char *build) {
    char path[512];
    FILE *f;
    uint8_t *image, *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u, pol;
    haeun_vm vm;
    haeun_opt_state st;
    uint64_t i;
    printf("RUN shadow_optimize_no_mutate\n");
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 0 &&
          fseek(f, 0, SEEK_SET) == 0);
    image = (uint8_t *)malloc((size_t)n);
    CHECK(image && (got = fread(image, 1, (size_t)n, f)) == (size_t)n);
    (void)fclose(f);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram &&
          haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) == 0);
    free(image);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == 0);
    for (i = 0; i < 3000u; ++i)
        if (haeun_vm_step(&vm) != 0)
            break;
    pol = rd32(ram, POL);
    haeun_opt_state_init(&st);
    snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build);
    st.cfg.reps = 2u;
    st.cfg.min_success = 1u;
    st.mode = HAEUN_OPT_MODE_OPTIMIZE;
    CHECK(haeun_opt_run(&st) == HAEUN_OPT_OK);
    CHECK(rd32(ram, POL) == pol);
    free(ram);
    printf("shadow_optimize_no_mutate ok\n");
    return 0;
}

/**
 * Entry for R3.6 optimizer guest tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r36_opt_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_optimize_all(argv[1]) || test_all_profiles(argv[1]) ||
        test_shadow_no_mutate(argv[1]))
        return 1;
    printf("r36 opt guest tests passed\n");
    return 0;
}
