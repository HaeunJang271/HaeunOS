#include "haeun/mmu.h"
#include "haeun/pt.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define ENC(op, rd, ra, rb, imm)                                               \
    (((uint32_t)(op) << 26) | ((uint32_t)(rd) << 22) | ((uint32_t)(ra) << 18) | \
     ((uint32_t)(rb) << 14) | ((uint32_t)(imm) & 0x3ffffu))
#define CSR_IMM(funct, csr)                                                    \
    (((uint32_t)(funct) << 12) | ((uint32_t)(csr) & 0xfffu))

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
 * Allowed YIELD keeps r1 and still enters the trap vector.
 */
static int test_ecall_yield_allowed(void) {
    uint8_t ram[64];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x10, 0, 1, 0, CSR_IMM(1, 1))); /* CSRRW trapvec */
    put_le32(ram + 4u, ENC(0x0d, 0, 0, 0, 0));             /* ECALL */
    put_le32(ram + 16u, ENC(0x01, 0, 0, 0, 0));            /* HALT handler */
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.r[1] = 16u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_YIELD;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 16u);
    CHECK(vm.r[1] == HAEUN_SYS_YIELD);
    return 0;
}

/**
 * Unknown syscall number is poisoned before trap entry.
 */
static int test_ecall_unknown_poisoned(void) {
    uint8_t ram[64];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x10, 0, 1, 0, CSR_IMM(1, 1)));
    put_le32(ram + 4u, ENC(0x0d, 0, 0, 0, 0));
    put_le32(ram + 16u, ENC(0x01, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.r[1] = 16u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = 99u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 16u);
    CHECK(vm.r[1] == 0xffffffffu);
    return 0;
}

/**
 * EXEC to a non-U page is poisoned; trap still delivered.
 */
static int test_ecall_exec_no_u_poisoned(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    uint32_t l1 = 0x2000u;
    uint32_t l0 = 0x3000u;
    uint32_t code_pa = 0x4000u;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x10, 0, 1, 0, CSR_IMM(1, 1)));
    put_le32(ram + 4u, ENC(0x0d, 0, 0, 0, 0));
    put_le32(ram + 16u, ENC(0x01, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_pt_map_page(ram, sizeof(ram), l1, l0, 0x1000u, code_pa,
                            HAEUN_PTE_V | HAEUN_PTE_R | HAEUN_PTE_X));
    /* User-executable identity map for the ECALL site; EXEC target lacks U. */
    CHECK(haeun_pt_map_page(ram, sizeof(ram), l1, l0, 0u, 0u,
                            HAEUN_PTE_V | HAEUN_PTE_R | HAEUN_PTE_W |
                                HAEUN_PTE_X | HAEUN_PTE_U));
    vm.csr_ptbr = haeun_pt_make_ptbr(l1);
    vm.r[1] = 16u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_EXEC;
    vm.r[2] = 0x1000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 16u);
    CHECK(vm.r[1] == 0xffffffffu);
    return 0;
}

int main(void) {
    if (test_ecall_yield_allowed() != 0) return 1;
    if (test_ecall_unknown_poisoned() != 0) return 1;
    if (test_ecall_exec_no_u_poisoned() != 0) return 1;
    puts("all sec ecall tests passed");
    return 0;
}
