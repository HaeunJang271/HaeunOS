#include "haeun/vm.h"
#include "haeun/mmu.h"
#include "haeun/tlb.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))
#define CSR_IMM(funct,csr) (((uint32_t)(funct)<<12)|((uint32_t)(csr)&0xfffu))

static void put_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static void map_leaf(uint8_t *ram, uint32_t l0_pa, uint32_t vpn0, uint32_t ppn,
                     uint32_t flags) {
    put_le32(ram + l0_pa + vpn0 * 4u, (ppn << 12) | flags);
}

static int test_identity_map_execute_and_load(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    /* L1[0] -> L0; L0[0] RX page0; L0[1] RW page1 */
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u);
    map_leaf(ram,l0,1u,1u,1u|2u|4u);
    put_le32(ram+0x1000u, 0x11223344u);
    /* 0: CSRRW ptbr; 4: LW from VA 0x1000; 8: HALT */
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,6)));
    put_le32(ram+4u, ENC(8,2,3,0,0));
    put_le32(ram+8u, ENC(1,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=l1|1u;
    vm.r[3]=0x1000u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.csr_ptbr==(l1|1u));
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[2]==0x11223344u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

static int test_page_fault_trap(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u);
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,1))); /* trapvec */
    put_le32(ram+4u, ENC(0x10,0,4,0,CSR_IMM(1,6))); /* ptbr */
    put_le32(ram+8u, ENC(8,2,3,0,0));               /* LW unmapped */
    put_le32(ram+12u, ENC(1,0,0,0,0));
    put_le32(ram+32u, ENC(0x10,5,0,0,CSR_IMM(2,3))); /* cause */
    put_le32(ram+36u, ENC(0x10,6,0,0,CSR_IMM(2,4))); /* tval */
    put_le32(ram+40u, ENC(1,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=32u;
    vm.r[4]=l1|1u;
    vm.r[3]=0x8000u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(vm.pc==32u&&vm.csr_cause==HAEUN_CAUSE_PAGE_FAULT&&vm.csr_tval==0x8000u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[5]==5u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[6]==0x8000u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

static int test_user_requires_u_bit(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u); /* no U */
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=l1|1u;
    vm.privilege=HAEUN_PRIV_USER;
    CHECK(!haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause));
    CHECK(cause==HAEUN_CAUSE_PAGE_FAULT);
    vm.privilege=HAEUN_PRIV_SUPERVISOR;
    CHECK(haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u|16u);
    haeun_tlb_flush(&vm);
    vm.privilege=HAEUN_PRIV_USER;
    CHECK(haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0u);
    return 0;
}

static int test_bare_mode_unchanged(void) {
    uint8_t ram[16];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    put_le32(ram, ENC(1,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    CHECK(vm.csr_ptbr==0u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

int main(void) {
    if (test_identity_map_execute_and_load()!=0) return 1;
    if (test_page_fault_trap()!=0) return 1;
    if (test_user_requires_u_bit()!=0) return 1;
    if (test_bare_mode_unchanged()!=0) return 1;
    puts("all MMU v1 translation tests passed");
    return 0;
}
