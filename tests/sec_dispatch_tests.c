#include "haeun/hxf.h"
#include "haeun/mmu.h"
#include "haeun/pt.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * Allowed YIELD and denied unknown number go through dispatch.
 */
static int test_dispatch_syscall_numbers(void) {
    haeun_sec_syscall_req req;
    memset(&req, 0, sizeof(req));
    req.privilege = HAEUN_PRIV_USER;
    req.number = HAEUN_SYS_YIELD;
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_OK);
    req.number = HAEUN_SYS_TRYWAIT;
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_OK);
    req.number = (uint32_t)HAEUN_SYS_MAX + 1u;
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_DENIED);
    return 0;
}

/**
 * EXEC with garbage image is denied before any load.
 */
static int test_dispatch_exec_image(void) {
    haeun_sec_syscall_req req;
    uint8_t bad[64];
    uint8_t image[256];
    uint8_t seg[4] = {0x00, 0x00, 0x00, 0x04};
    haeun_hxf_segment s;
    size_t out_size = 0u;
    memset(bad, 0, sizeof(bad));
    memset(&req, 0, sizeof(req));
    req.privilege = HAEUN_PRIV_USER;
    req.number = HAEUN_SYS_EXEC;
    req.arg0 = 0x1000u;
    req.user_lo = 0x1000u;
    req.user_hi = 0x2000u;
    req.exec_image = bad;
    req.exec_size = sizeof(bad);
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_DENIED);
    memset(&s, 0, sizeof(s));
    s.flags = 5u;
    s.virtual_address = 0x1000u;
    s.file_data = seg;
    s.file_size = 4u;
    s.memory_size = 8u;
    s.alignment = 4u;
    CHECK(haeun_hxf_write(&s, 1u, 0x1000u, image, sizeof(image), &out_size) ==
          HAEUN_HXF_OK);
    req.exec_image = image;
    req.exec_size = out_size;
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_OK);
    return 0;
}

/**
 * User EXEC entry VA without U permission is denied via MMU.
 */
static int test_dispatch_exec_mmu_u(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_sec_syscall_req req;
    uint32_t l1 = 0x2000u;
    uint32_t l0 = 0x3000u;
    uint32_t code_pa = 0x4000u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_pt_map_page(ram, sizeof(ram), l1, l0, 0x1000u, code_pa,
                            HAEUN_PTE_V | HAEUN_PTE_R | HAEUN_PTE_X));
    vm.csr_ptbr = haeun_pt_make_ptbr(l1);
    memset(&req, 0, sizeof(req));
    req.privilege = HAEUN_PRIV_USER;
    req.number = HAEUN_SYS_EXEC;
    req.arg0 = 0x1000u;
    req.vm = &vm;
    /* No image → skip image check; MMU must still deny missing U. */
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_DENIED);
    CHECK(haeun_pt_map_page(ram, sizeof(ram), l1, l0, 0x1000u, code_pa,
                            HAEUN_PTE_V | HAEUN_PTE_R | HAEUN_PTE_X |
                                HAEUN_PTE_U));
    vm.tlb_next = 0u;
    memset(vm.tlb_valid, 0, sizeof(vm.tlb_valid));
    CHECK(haeun_sec_dispatch(&req) == HAEUN_SEC_OK);
    return 0;
}

int main(void) {
    if (test_dispatch_syscall_numbers() != 0) return 1;
    if (test_dispatch_exec_image() != 0) return 1;
    if (test_dispatch_exec_mmu_u() != 0) return 1;
    puts("all sec dispatch tests passed");
    return 0;
}
