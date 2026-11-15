#include "haeun/guest_pcb.h"
#include "haeun/hxf.h"
#include "haeun/net_sock.h"
#include "haeun/vm.h"
#include "haeun/vm_net_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (16u * 1024u * 1024u)
#define PCB0 0x0000A000u

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
    FILE *f = fopen(path, "rb");
    long n;
    uint8_t *buf;
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
 * Runs process_net.hxf under --net semantics; checks PCB, TX, inject, cleanup.
 */
int main(int argc, char **argv) {
    uint8_t *image;
    uint8_t *ram;
    size_t image_size = 0u;
    uint32_t entry = 0u, isa = 0u;
    haeun_vm vm;
    haeun_vm_net_host host;
    uint64_t steps = 0u;
    int saw_user = 0;
    int i;

    if (argc != 2) {
        fprintf(stderr, "usage: m9_net_probe <process_net.hxf>\n");
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
    CHECK(haeun_vm_net_host_attach(&host, &vm) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_host_run(&host, &vm, 1000000u, &steps) ==
          HAEUN_VM_HALTED);
    CHECK(host.tx.count >= 1);
    CHECK(host.injected == 1);
    CHECK(host.tx.len > 14u);
    CHECK(vm.r[1] == 1u);
    CHECK(rd32(ram, PCB0 + 0u) == 5u);
    CHECK(rd32(ram, PCB0 + 4u) == 1u);
    for (i = 0; i < HAEUN_NET_MAX_SOCKS; ++i)
        CHECK(!host.ss.socks[i].used);
    (void)saw_user;
    puts("net_ok");
    free(ram);
    return 0;
}
