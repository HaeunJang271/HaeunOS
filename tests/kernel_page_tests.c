#include "haeun/mmu.h"
#include "haeun/pt.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

/**
 * Verifies the identity-map layout the guest kernel installs: kernel page
 * without U, user page with U, and IRQC/timer/UART leaves.
 */
int main(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u, l0_io=0x4000u;
    memset(ram,0,sizeof(ram));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0,0x00100000u,0x00100000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_X));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0,0x00101000u,0x00101000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_X|HAEUN_PTE_U));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0_io,0x10000000u,0x10000000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_W));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0_io,0x10001000u,0x10001000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_W));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0_io,0x10002000u,0x10002000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_W));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=haeun_pt_make_ptbr(l1);
    vm.privilege=HAEUN_PRIV_SUPERVISOR;
    CHECK(haeun_mmu_translate(&vm,0x00100000u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&
          pa==0x00100000u);
    vm.privilege=HAEUN_PRIV_USER;
    CHECK(!haeun_mmu_translate(&vm,0x00100000u,HAEUN_MMU_ACCESS_X,&pa,&cause));
    CHECK(cause==HAEUN_CAUSE_PAGE_FAULT);
    CHECK(haeun_mmu_translate(&vm,0x00101000u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&
          pa==0x00101000u);
    vm.privilege=HAEUN_PRIV_SUPERVISOR;
    CHECK(haeun_mmu_translate(&vm,0x10000000u,HAEUN_MMU_ACCESS_W,&pa,&cause)&&
          pa==0x10000000u);
    CHECK(haeun_mmu_translate(&vm,0x10001000u,HAEUN_MMU_ACCESS_W,&pa,&cause)&&
          pa==0x10001000u);
    CHECK(haeun_mmu_translate(&vm,0x10002000u,HAEUN_MMU_ACCESS_W,&pa,&cause)&&
          pa==0x10002000u);
    puts("all kernel identity-map layout tests passed");
    return 0;
}
