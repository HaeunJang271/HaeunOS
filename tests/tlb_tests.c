#include "haeun/mmu.h"
#include "haeun/tlb.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))

static void put_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static void map_leaf(uint8_t *ram, uint32_t l0_pa, uint32_t vpn0, uint32_t ppn,
                     uint32_t flags) {
    put_le32(ram + l0_pa + vpn0 * 4u, (ppn << 12) | flags);
}

/**
 * After a successful walk fills the TLB, corrupting the PTE in RAM must not
 * change the cached translation until ptbr is rewritten.
 */
static int test_tlb_hit_ignores_stale_pte(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=l1|1u;
    CHECK(haeun_mmu_translate(&vm,0x100u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0x100u);
    map_leaf(ram,l0,0u,0u,0u); /* invalidate leaf in RAM */
    CHECK(haeun_mmu_translate(&vm,0x100u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0x100u);
    return 0;
}

/**
 * Rewriting ptbr flushes the TLB so a cleared PTE becomes visible.
 */
static int test_ptbr_write_flushes_tlb(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=l1|1u;
    CHECK(haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0u);
    map_leaf(ram,l0,0u,0u,0u);
    haeun_tlb_flush(&vm);
    CHECK(!haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause));
    CHECK(cause==HAEUN_CAUSE_PAGE_FAULT);
    return 0;
}

/**
 * Failed translations must not leave a TLB entry behind.
 */
static int test_fault_does_not_fill(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u, pte=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=l1|1u;
    CHECK(!haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_R,&pa,&cause));
    CHECK(!haeun_tlb_lookup(&vm,0u,&pte));
    return 0;
}

/**
 * Supervisor SFENCE flushes cached translations so a cleared PTE faults.
 */
static int test_sfence_flushes_tlb(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    put_le32(ram+l1, (l0 >> 12 << 12) | 1u);
    map_leaf(ram,l0,0u,0u,1u|2u|8u);
    put_le32(ram+0u, ENC(0x12,0,0,0,0)); /* SFENCE */
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=l1|1u;
    CHECK(haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0u);
    map_leaf(ram,l0,0u,0u,0u); /* RAM leaf gone; TLB still caches until SFENCE */
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* fetch hits TLB, then flushes */
    CHECK(!haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause));
    CHECK(cause==HAEUN_CAUSE_PAGE_FAULT);
    return 0;
}

/**
 * User-mode SFENCE takes a privilege fault (cause 4).
 */
static int test_user_sfence_privilege_fault(void) {
    uint8_t ram[0x1000];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    put_le32(ram+0u, ENC(0x12,0,0,0,0));
    put_le32(ram+32u, ENC(1,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.privilege=HAEUN_PRIV_USER;
    vm.csr_trapvec=32u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(vm.pc==32u&&vm.csr_cause==4u&&vm.privilege==HAEUN_PRIV_SUPERVISOR);
    return 0;
}

int main(void) {
    if (test_tlb_hit_ignores_stale_pte()!=0) return 1;
    if (test_ptbr_write_flushes_tlb()!=0) return 1;
    if (test_fault_does_not_fill()!=0) return 1;
    if (test_sfence_flushes_tlb()!=0) return 1;
    if (test_user_sfence_privilege_fault()!=0) return 1;
    puts("all TLB v1 tests passed");
    return 0;
}
