#include "haeun/hfs.h"
#include "haeun/proc_hfs.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define ENC(op, rd, ra, rb, imm)                                               \
    (((uint32_t)(op) << 26) | ((uint32_t)(rd) << 22) | ((uint32_t)(ra) << 18) | \
     ((uint32_t)(rb) << 14) | ((uint32_t)(imm) & 0x3ffffu))

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

/**
 * Guest ECALL OPEN with a RAM path creates/writes via the process-HFS bridge.
 */
static int test_ecall_open_path_roundtrip(void) {
    uint8_t ram[4096];
    uint8_t disk[64u * 1024u];
    haeun_vm vm;
    haeun_hfs fs;
    haeun_proc_ft ft;
    uint32_t fd;
    uint8_t b = 0u;
    uint32_t n = 0u;
    memset(ram, 0, sizeof(ram));
    memcpy(ram + 0x200u, "note.txt", 9u);
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0)); /* ECALL */
    put_le32(ram + 4u, ENC(0x01, 0, 0, 0, 0)); /* HALT */
    CHECK(haeun_hfs_mkfs(disk, sizeof(disk), 32u) == HAEUN_HFS_OK);
    CHECK(haeun_hfs_mount(&fs, disk, sizeof(disk)) == HAEUN_HFS_OK);
    haeun_proc_ft_init(&ft, &fs);
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    haeun_vm_set_ecall_io(&vm, haeun_proc_ecall_io, &ft);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_OPEN;
    vm.r[2] = 0x200u;
    vm.r[3] = 1u; /* create */
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 4u); /* completed without trap */
    fd = vm.r[1];
    CHECK(fd < HAEUN_PROC_NFD);
    CHECK(ft.fds[fd].kind == HAEUN_PROC_FD_FILE);
    vm.r[1] = HAEUN_SYS_WRITE;
    vm.r[2] = fd;
    vm.r[3] = (uint32_t)'Z';
    vm.pc = 0u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    CHECK(haeun_proc_close(&ft, fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_open_path(&ft, "note.txt", 0u, &fd) == HAEUN_HFS_OK);
    CHECK(haeun_proc_read(&ft, fd, &b, 1u, &n) == HAEUN_HFS_OK);
    CHECK(n == 1u && b == (uint8_t)'Z');
    CHECK(haeun_hfs_unmount(&fs) == HAEUN_HFS_OK);
    return 0;
}

/**
 * YIELD still takes the architectural trap when the I/O hook is installed.
 */
static int test_yield_still_traps(void) {
    uint8_t ram[64];
    haeun_vm vm;
    haeun_proc_ft ft;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.csr_trapvec = 32u;
    haeun_proc_ft_init(&ft, NULL);
    haeun_vm_set_ecall_io(&vm, haeun_proc_ecall_io, &ft);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_YIELD;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 32u);
    CHECK(vm.csr_cause == 3u);
    return 0;
}

int main(void) {
    if (test_ecall_open_path_roundtrip() != 0) return 1;
    if (test_yield_still_traps() != 0) return 1;
    puts("all proc ecall io tests passed");
    return 0;
}
