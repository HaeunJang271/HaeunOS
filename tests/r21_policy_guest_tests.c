/**
 * R2.1 guest policy_select read-only call and R1.3 inspector compatibility.
 * Usage: r21_policy_guest_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/policy_verify.h"
#include "haeun/telem_inspect.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define STRIDE 0x100u
#define CURR 0x0000A400u
#define POL 0x0000A474u
#define T_IRQ 0x0000A464u
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

static int open_yield(const char *dir, haeun_vm *vm, uint8_t *ram) {
    char path[512];
    FILE *f;
    uint8_t *image;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", dir);
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

/** Guest policy_select is read-only on PCB page and timer CSRs. */
static int test_guest_readonly(const char *dir) {
    uint8_t *ram;
    haeun_vm vm;
    uint8_t snap[SNAP_LEN];
    uint32_t pol_va, slot = 0u;
    uint64_t i, tcnt, tcmp;
    uint32_t tctl;
    haeun_vm_result st;
    haeun_policy_fixture fx;
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN guest_readonly\n");
    pol_va = load_sym(dir, "policy_select");
    CHECK(pol_va != 0u);
    CHECK(open_yield(dir, &vm, ram) == 0);
    for (i = 0; i < 20000u; ++i) {
        st = haeun_vm_step(&vm);
        if (vm.privilege == HAEUN_PRIV_USER)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(vm.privilege == HAEUN_PRIV_USER);
    wr32(ram, PCB + 0u, 1u);
    wr32(ram, PCB + STRIDE, 3u);
    wr32(ram, PCB + 2u * STRIDE, 1u);
    wr32(ram, PCB + 3u * STRIDE, 1u);
    wr32(ram, CURR, 0u);
    wr32(ram, POL, 0u);
    wr32(ram, TRAMP, HALT_WORD);
    memcpy(snap, ram + PCB, SNAP_LEN);
    /* Disarm so a due compare cannot mutate control mid-probe. */
    vm.timer_control = 0u;
    tcnt = vm.timer_count;
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
    CHECK(vm.r[1] == 1u && vm.r[6] == 2u);
    CHECK(memcmp(ram + PCB, snap, SNAP_LEN) == 0);
    /* Policy has no timer stores; COUNT advances only via host retire. */
    CHECK(vm.timer_compare == tcmp && vm.timer_control == tctl);
    CHECK(vm.timer_count == tcnt + i + 1u);
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = 0u;
    fx.curr = 0u;
    fx.state[0] = 1u;
    fx.state[1] = 3u;
    fx.state[2] = 1u;
    fx.state[3] = 1u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 1);
    CHECK(slot == vm.r[6]);
    free(ram);
    printf("guest_readonly ok\n");
    return 0;
}

/** Invalid policy ID in guest returns r1=0 without RAM writes. */
static int test_guest_bad_id(const char *dir) {
    uint8_t *ram;
    haeun_vm vm;
    uint8_t snap[SNAP_LEN];
    uint32_t pol_va;
    uint64_t i;
    haeun_vm_result st;
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN guest_bad_id\n");
    pol_va = load_sym(dir, "policy_select");
    CHECK(pol_va != 0u);
    CHECK(open_yield(dir, &vm, ram) == 0);
    for (i = 0; i < 20000u; ++i) {
        st = haeun_vm_step(&vm);
        if (vm.privilege == HAEUN_PRIV_USER)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    wr32(ram, POL, 99u);
    wr32(ram, PCB + 0u, 1u);
    wr32(ram, TRAMP, HALT_WORD);
    memcpy(snap, ram + PCB, SNAP_LEN);
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
    CHECK(vm.r[1] == 0u);
    CHECK(memcmp(ram + PCB, snap, SNAP_LEN) == 0);
    free(ram);
    printf("guest_bad_id ok\n");
    return 0;
}

/** R1.3 text/JSON output remains compatible; 0xA474 not a counter field. */
static int test_r13_compat(const char *dir) {
    uint8_t *ram;
    haeun_vm vm;
    haeun_telem_report rep;
    haeun_vm_result st;
    uint64_t i;
    char buf[4096];
    FILE *mem;
    size_t n;
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    printf("RUN r13_compat\n");
    CHECK(open_yield(dir, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(haeun_telem_read(&vm, &rep) == HAEUN_TELEM_OK);
    CHECK(rep.timer_irqs == rd32(ram, T_IRQ));
    mem = tmpfile();
    CHECK(mem != NULL);
    haeun_telem_fprint(mem, &rep);
    haeun_telem_fprint_json(mem, &rep);
    rewind(mem);
    n = fread(buf, 1u, sizeof(buf) - 1u, mem);
    buf[n] = '\0';
    (void)fclose(mem);
    CHECK(strstr(buf, "timer_irqs") != NULL);
    CHECK(strstr(buf, "0xA474") == NULL);
    CHECK(strstr(buf, "\"slots\":[") != NULL);
    free(ram);
    printf("r13_compat ok\n");
    return 0;
}

/**
 * Runs R2.1 guest policy call and inspector compatibility tests.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r21_policy_guest_tests <build-dir>\n");
        return 2;
    }
    if (test_guest_readonly(argv[1]) || test_guest_bad_id(argv[1]) ||
        test_r13_compat(argv[1]))
        return 1;
    printf("r21 policy guest tests passed\n");
    return 0;
}
