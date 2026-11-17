/**
 * R1.3 read-only telemetry inspector tests.
 * Usage: r13_telem_inspect_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/telem_inspect.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB 0x0000A000u
#define STRIDE 0x100u
#define CURR 0x0000A400u
#define IDLE 0x0000A460u
#define T_IRQ 0x0000A464u
#define T_CTX 0x0000A468u
#define PAGE_BYTES 0x1000u

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

static int open_image(const char *path, haeun_vm *vm, uint8_t *ram) {
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

/** Planted RAM: address math, unsigned values, completion, FREE stale, slot 3. */
static int test_planted(void) {
    uint8_t *ram;
    haeun_vm vm;
    haeun_telem_report rep;
    uint8_t before[PAGE_BYTES];
    char buf[2048];
    FILE *mem;
    unsigned i;
    int saw_expected = 0;
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, T_IRQ, 7u);
    wr32(ram, T_CTX, 3u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 128u, 0xFFFFFFF0u);
    wr32(ram, PCB + 144u, 12u);
    wr32(ram, PCB + 152u, 0u);
    wr32(ram, PCB + STRIDE + 0u, 0u);
    wr32(ram, PCB + STRIDE + 4u, 0u);
    wr32(ram, PCB + STRIDE + 128u, 99u);
    wr32(ram, PCB + STRIDE + 144u, 5u);
    wr32(ram, PCB + STRIDE + 152u, 1u);
    wr32(ram, PCB + STRIDE + 148u, 0xABCDu);
    wr32(ram, PCB + 3u * STRIDE + 0u, 1u);
    wr32(ram, PCB + 3u * STRIDE + 4u, 99u);
    wr32(ram, PCB + 3u * STRIDE + 128u, 1u);
    memcpy(before, ram + PCB, PAGE_BYTES);
    vm.csr_epc = 0x00108000u;
    vm.csr_ptbr = 0x00002001u;
    vm.timer_count = 100u;
    vm.timer_compare = 196u;
    vm.timer_control = 1u;
    CHECK(haeun_telem_read(&vm, &rep) == HAEUN_TELEM_OK);
    CHECK(memcmp(ram + PCB, before, PAGE_BYTES) == 0);
    CHECK(vm.csr_epc == 0x00108000u);
    CHECK(vm.csr_ptbr == 0x00002001u);
    CHECK(vm.timer_count == 100u);
    CHECK(vm.timer_compare == 196u);
    CHECK(vm.timer_control == 1u);
    CHECK(rep.timer_irqs == 7u);
    CHECK(rep.context_switches == 3u);
    CHECK(rep.slots[0].run_ticks == 0xFFFFFFF0u);
    CHECK(rep.slots[0].dispatch_count == 12u);
    CHECK(rep.slots[0].complete_valid == 0u);
    CHECK(rep.slots[1].state == 0u);
    CHECK(rep.slots[1].run_ticks == 99u);
    CHECK(rep.slots[3].as_slot_valid == 0);
    CHECK(strcmp(haeun_telem_state_name(2u), "RUNNING") == 0);
    for (i = 0; i < rep.note_count; ++i) {
        if (rep.note_kind[i] == HAEUN_TELEM_NOTE_EXPECTED)
            saw_expected = 1;
        CHECK(rep.note_kind[i] != HAEUN_TELEM_NOTE_WARN);
    }
    CHECK(saw_expected);
    mem = tmpfile();
    CHECK(mem != NULL);
    haeun_telem_fprint(mem, &rep);
    rewind(mem);
    CHECK(fread(buf, 1u, sizeof(buf) - 1u, mem) > 0u);
    (void)fclose(mem);
    CHECK(strstr(buf, "4294967280") != NULL); /* 0xFFFFFFF0 unsigned */
    CHECK(strstr(buf, "Completion      : N/A") != NULL);
    CHECK(strstr(buf, "no AS") != NULL);
    CHECK(memcmp(ram + PCB, before, PAGE_BYTES) == 0);
    free(ram);
    printf("planted ok\n");
    return 0;
}

/** Live r12_run image: inspector matches direct RAM; read-only after halt. */
static int test_live(const char *dir) {
    char path[512];
    uint8_t *ram;
    haeun_vm vm;
    haeun_telem_report rep;
    haeun_vm_result st = HAEUN_VM_OK;
    uint8_t before[PAGE_BYTES];
    uint64_t i;
    uint32_t epc, ptbr, sp, tctl;
    uint64_t tcnt, tcmp;
    snprintf(path, sizeof(path), "%s/r12_run.hxf", dir);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(open_image(path, &vm, ram) == 0);
    for (i = 0; i < 200000u; ++i) {
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    memcpy(before, ram + PCB, PAGE_BYTES);
    epc = vm.csr_epc;
    ptbr = vm.csr_ptbr;
    sp = rd32(ram, PCB + 116u);
    tcnt = vm.timer_count;
    tcmp = vm.timer_compare;
    tctl = vm.timer_control;
    CHECK(haeun_telem_read(&vm, &rep) == HAEUN_TELEM_OK);
    CHECK(rep.timer_irqs == rd32(ram, T_IRQ));
    CHECK(rep.context_switches == rd32(ram, T_CTX));
    CHECK(rep.idle_flag == rd32(ram, IDLE));
    CHECK(rep.curr_index == rd32(ram, CURR));
    CHECK(rep.slots[0].run_ticks == rd32(ram, PCB + 128u));
    CHECK(rep.slots[0].dispatch_count == rd32(ram, PCB + 144u));
    CHECK(rep.slots[0].complete_valid == rd32(ram, PCB + 152u));
    if (rep.slots[0].complete_valid != 0u)
        CHECK(rep.slots[0].complete_lo == rd32(ram, PCB + 148u));
    CHECK(rep.slots[3].as_slot_valid == 0);
    haeun_telem_fprint(stdout, &rep);
    CHECK(memcmp(ram + PCB, before, PAGE_BYTES) == 0);
    CHECK(vm.csr_epc == epc);
    CHECK(vm.csr_ptbr == ptbr);
    CHECK(rd32(ram, PCB + 116u) == sp);
    CHECK(rd32(ram, CURR) == rd32(before, CURR - PCB));
    CHECK(vm.timer_count == tcnt);
    CHECK(vm.timer_compare == tcmp);
    CHECK(vm.timer_control == tctl);
    free(ram);
    printf("live ok\n");
    return 0;
}

/**
 * Runs R1.3 inspector unit and live read-only checks.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: r13_telem_inspect_tests <build-dir>\n");
        return 2;
    }
    printf("RUN planted\n");
    if (test_planted() != 0)
        return 1;
    printf("RUN live\n");
    if (test_live(argv[1]) != 0)
        return 1;
    printf("r13 telem inspect tests passed\n");
    return 0;
}
