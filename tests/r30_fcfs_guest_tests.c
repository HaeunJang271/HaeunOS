/**
 * R3.0 guest FCFS: boot policy, read-only policy_select, eval RR vs FCFS.
 * Usage: r30_fcfs_guest_tests <build-dir>
 */
#include "haeun/eval_compare.h"
#include "haeun/eval_report.h"
#include "haeun/eval_runner.h"
#include "haeun/eval_workload.h"
#include "haeun/hxf.h"
#include "haeun/policy_verify.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define STRIDE 0x100u
#define POL 0x0000A474u
#define SNAP_LEN 0x480u
#define TRAMP 0x00107ff0u
#define HALT_WORD 0x04000000u

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

static uint32_t load_sym(const char *dir, const char *name) {
    char path[512];
    char line[128];
    char sym[64];
    unsigned va = 0u;
    FILE *f;
    snprintf(path, sizeof(path), "%s/r21-symbols.txt", dir);
    f = fopen(path, "r");
    if (f == NULL)
        return 0u;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (sscanf(line, "%63s 0x%x", sym, &va) == 2 && strcmp(sym, name) == 0) {
            (void)fclose(f);
            return (uint32_t)va;
        }
    }
    (void)fclose(f);
    return 0u;
}

static int open_fcfs_yield(const char *dir, haeun_vm *vm, uint8_t *ram) {
    char path[512];
    FILE *f;
    uint8_t *image;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    snprintf(path, sizeof(path), "%s/fcfs/r12_yield.hxf", dir);
    f = fopen(path, "rb");
    if (f == NULL)
        return 1;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
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
    return haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK ? 0
                                                                                : 1;
}

static int test_boot_and_readonly(const char *dir) {
    uint8_t *ram;
    haeun_vm vm;
    uint8_t snap[SNAP_LEN];
    uint32_t pol_va;
    uint64_t i;
    uint32_t tctl;
    uint64_t tcmp;
    haeun_vm_result st;
    haeun_policy_fixture fx;
    uint32_t slot = 0u;
    printf("RUN boot_readonly\n");
    pol_va = load_sym(dir, "policy_select");
    CHECK(pol_va != 0u);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(open_fcfs_yield(dir, &vm, ram) == 0);
    for (i = 0; i < 20000u; ++i) {
        st = haeun_vm_step(&vm);
        if (vm.privilege == HAEUN_PRIV_USER)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(vm.privilege == HAEUN_PRIV_USER);
    CHECK(rd32(ram, POL) == 1u);
    wr32(ram, PCB + 0u, 1u);
    wr32(ram, PCB + STRIDE, 1u);
    wr32(ram, PCB + 2u * STRIDE, 1u);
    wr32(ram, PCB + 160u, 200u);
    wr32(ram, PCB + STRIDE + 160u, 50u);
    wr32(ram, PCB + 2u * STRIDE + 160u, 100u);
    wr32(ram, TRAMP, HALT_WORD);
    memcpy(snap, ram + PCB, SNAP_LEN);
    vm.timer_control = 0u;
    tcmp = vm.timer_compare;
    tctl = vm.timer_control;
    vm.privilege = HAEUN_PRIV_SUPERVISOR;
    vm.csr_status = 0u;
    vm.r[13] = 0u;
    vm.r[15] = TRAMP;
    vm.pc = pol_va;
    for (i = 0; i < 5000u; ++i) {
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 1u && vm.r[6] == 1u);
    CHECK(memcmp(ram + PCB, snap, SNAP_LEN) == 0);
    CHECK(vm.timer_compare == tcmp && vm.timer_control == tctl);
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_FCFS;
    fx.state[0] = 1u;
    fx.state[1] = 1u;
    fx.state[2] = 1u;
    fx.ready_since[0] = 200u;
    fx.ready_since[1] = 50u;
    fx.ready_since[2] = 100u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 1 && slot == 1u);
    free(ram);
    printf("boot_readonly ok\n");
    return 0;
}

static int test_eval_both(const char *dir) {
    haeun_eval_workload wl;
    haeun_eval_experiment err, efc;
    haeun_eval_report_bundle br, bf;
    printf("RUN eval_both\n");
    CHECK(haeun_eval_workload_get(HAEUN_WL_SHORT_JOB, &wl) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_experiment_run(&wl, dir, HAEUN_EVAL_POLICY_RR, "rr_short", 2u,
                                    &err) == HAEUN_EVAL_OK);
    CHECK(err.fail_count == 0u && err.policy_id == 0u);
    CHECK(haeun_eval_experiment_run(&wl, dir, HAEUN_EVAL_POLICY_FCFS, "fcfs_short",
                                    2u, &efc) == HAEUN_EVAL_OK);
    CHECK(efc.fail_count == 0u && efc.policy_id == 1u);
    CHECK(haeun_eval_report_prepare(&err, NULL, &br) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_report_prepare(&efc, NULL, &bf) == HAEUN_EVAL_OK);
    CHECK(haeun_eval_compare(&br, &bf, stdout) == HAEUN_EVAL_OK);
    printf("eval_both ok\n");
    return 0;
}

/**
 * Entry for R3.0 guest FCFS and evaluation tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r30_fcfs_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_boot_and_readonly(argv[1]) || test_eval_both(argv[1]))
        return 1;
    printf("r30 fcfs guest tests passed\n");
    return 0;
}
