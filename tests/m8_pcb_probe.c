#include "haeun/hxf.h"
#include "haeun/vm.h"
#include "haeun/vm_gui_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB0 0x0000A000u
#define CURR 0x0000A400u

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

static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *f;
    long n;
    uint8_t *buf;
    f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return NULL;
    }
    buf = (uint8_t *)malloc((size_t)n);
    if (buf == NULL) {
        (void)fclose(f);
        return NULL;
    }
    if (fread(buf, 1u, (size_t)n, f) != (size_t)n) {
        free(buf);
        (void)fclose(f);
        return NULL;
    }
    (void)fclose(f);
    *out_size = (size_t)n;
    return buf;
}

/**
 * Loads process_gui.hxf, attaches GUI host assist, runs to halt, checks PCB0.
 */
int main(int argc, char **argv) {
    uint8_t *image;
    uint8_t *ram;
    size_t image_size = 0u;
    uint32_t entry = 0u;
    uint32_t isa = 0u;
    haeun_vm vm;
    haeun_vm_gui_host gui;
    uint64_t steps = 0u;
    int saw_user = 0;
    haeun_vm_result st;

    if (argc != 2) {
        fprintf(stderr, "usage: m8_pcb_probe <process_gui.hxf>\n");
        return 2;
    }
    image = read_file(argv[1], &image_size);
    CHECK(image != NULL);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram != NULL);
    CHECK(haeun_hxf_load_ex(image, image_size, ram, RAM_SIZE, &entry, &isa) ==
          HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(&vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    CHECK(haeun_vm_gui_host_attach(&gui, &vm) == HAEUN_GFX_OK);

    while (steps < 1000000u) {
        st = haeun_vm_step(&vm);
        ++steps;
        if (vm.privilege == HAEUN_PRIV_USER)
            saw_user = 1;
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(st == HAEUN_VM_HALTED);
    CHECK(saw_user);
    CHECK(rd32(ram, PCB0 + 0u) == 5u);  /* ZOMBIE */
    CHECK(rd32(ram, PCB0 + 4u) == 1u);  /* pid */
    CHECK(rd32(ram, PCB0 + 16u) == 1u); /* exit_code == GETPID */
    CHECK(rd32(ram, CURR) == 0u);
    CHECK(vm.r[1] == 1u);
    puts("pcb_ok");
    free(ram);
    return 0;
}
