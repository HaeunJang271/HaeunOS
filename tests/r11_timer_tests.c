/**
 * R1.1 timer compare carry and signed sleep-deadline regression.
 * Usage: r11_timer_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u

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

static int load_image(const char *path, uint8_t **image, size_t *n) {
    FILE *f;
    long len;
    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "open failed %s\n", path);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "size failed %s\n", path);
        (void)fclose(f);
        return 1;
    }
    *image = (uint8_t *)malloc((size_t)len);
    if (*image == NULL || fread(*image, 1u, (size_t)len, f) != (size_t)len) {
        fprintf(stderr, "read failed %s\n", path);
        (void)fclose(f);
        free(*image);
        return 1;
    }
    (void)fclose(f);
    *n = (size_t)len;
    return 0;
}

/** Boot at COUNT_LO 0xFFFFFFF0. A user spin must take timer IRQs after wrap. */
static int test_wrap(const char *dir) {
    char path[512];
    uint8_t *image = NULL;
    uint8_t *ram;
    size_t n = 0u;
    uint32_t entry = 0u;
    uint32_t isa = 0u;
    haeun_vm vm;
    haeun_vm_result st;
    uint64_t i;
    uint32_t prev;
    unsigned traps = 0u;
    int saw_hi = 0;
    snprintf(path, sizeof(path), "%s/r11_spin.hxf", dir);
    printf("RUN wrap\n");
    CHECK(load_image(path, &image, &n) == 0);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_hxf_load_ex(image, n, ram, RAM_SIZE, &entry, &isa) == HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    vm.timer_count = UINT64_C(0xFFFFFFF0);
    vm.timer_compare = 0u;
    vm.timer_control = 0u;
    vm.irq_pending = 0u;
    for (i = 0; i < 8000u; ++i) {
        prev = vm.privilege;
        st = haeun_vm_step(&vm);
        if (prev == HAEUN_PRIV_USER && vm.privilege != HAEUN_PRIV_USER &&
            vm.csr_cause == 16u)
            traps++;
        if ((vm.timer_compare >> 32) != 0u)
            saw_hi = 1;
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    printf("wrap traps=%u compare=0x%llx count=0x%llx hi_seen=%d\n", traps,
           (unsigned long long)vm.timer_compare, (unsigned long long)vm.timer_count,
           saw_hi);
    CHECK(traps >= 3u);
    CHECK(saw_hi);
    CHECK(vm.timer_count > UINT64_C(0x100000000));
    free(ram);
    printf("wrap ok\n");
    return 0;
}

/**
 * Sleep whose deadline crosses 0x80000000 must not resume early.
 * COUNT is planted on the sleep ECALL so the store still sits below the sign bit.
 */
static int test_sign(const char *dir) {
    char path[512];
    uint8_t *image = NULL;
    uint8_t *ram;
    size_t n = 0u;
    uint32_t entry = 0u;
    uint32_t isa = 0u;
    haeun_vm vm;
    haeun_vm_result st;
    uint64_t i;
    int armed = 0;
    int saw_sleep = 0;
    uint32_t deadline = 0u;
    uint32_t now_at_store = 0u;
    snprintf(path, sizeof(path), "%s/r11_sign.hxf", dir);
    printf("RUN sign\n");
    CHECK(load_image(path, &image, &n) == 0);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_hxf_load_ex(image, n, ram, RAM_SIZE, &entry, &isa) == HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    for (i = 0; i < 400000u; ++i) {
        if (!armed && vm.privilege == HAEUN_PRIV_USER && vm.pc == 0x00108008u) {
            vm.timer_count = UINT64_C(0x7FFFFF4C);
            vm.timer_compare = vm.timer_count + 32u;
            vm.timer_control = 1u;
            vm.irq_pending = 0u;
            armed = 1;
        }
        st = haeun_vm_step(&vm);
        if (armed && !saw_sleep && rd32(ram, PCB + 20u) != 0u) {
            deadline = rd32(ram, PCB + 20u);
            now_at_store = (uint32_t)vm.timer_count;
            saw_sleep = 1;
            printf("sign now=0x%08x deadline=0x%08x\n", now_at_store, deadline);
        }
        if (saw_sleep && rd32(ram, PCB) == 3u && vm.privilege == HAEUN_PRIV_USER) {
            fprintf(stderr, "sleeper resumed early count=0x%llx\n",
                    (unsigned long long)vm.timer_count);
            free(ram);
            return 1;
        }
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(armed);
    CHECK(saw_sleep);
    CHECK((now_at_store & 0x80000000u) == 0u);
    CHECK((deadline & 0x80000000u) != 0u);
    CHECK((int32_t)(now_at_store - deadline) < 0);
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(vm.r[1] == 7u);
    CHECK((int32_t)((uint32_t)vm.timer_count - deadline) >= 0);
    free(ram);
    printf("sign ok\n");
    return 0;
}

/**
 * Runs the timer wrap and signed-deadline images.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r11_timer_tests <build-dir>\n");
        return 2;
    }
    if (test_wrap(argv[1]) || test_sign(argv[1]))
        return 1;
    printf("r11 timer tests passed\n");
    return 0;
}
