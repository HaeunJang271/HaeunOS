#include "haeun/mmu.h"
#include "haeun/pt.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

int main(void) {
    uint8_t ram[0x5000];
    haeun_vm vm;
    uint32_t pa=0u, cause=0u;
    const uint32_t l1=0x2000u, l0=0x3000u;
    memset(ram,0,sizeof(ram));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0,0u,0u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_X));
    CHECK(haeun_pt_map_page(ram,sizeof(ram),l1,l0,0x1000u,0x1000u,
                            HAEUN_PTE_V|HAEUN_PTE_R|HAEUN_PTE_W));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.csr_ptbr=haeun_pt_make_ptbr(l1);
    CHECK(haeun_mmu_translate(&vm,0u,HAEUN_MMU_ACCESS_X,&pa,&cause)&&pa==0u);
    CHECK(haeun_mmu_translate(&vm,0x1004u,HAEUN_MMU_ACCESS_R,&pa,&cause)&&pa==0x1004u);
    CHECK(!haeun_mmu_translate(&vm,0x2000u,HAEUN_MMU_ACCESS_R,&pa,&cause));
    CHECK(cause==HAEUN_CAUSE_PAGE_FAULT);
    CHECK(!haeun_pt_map_page(ram,sizeof(ram),l1,l0,1u,0u,HAEUN_PTE_V|HAEUN_PTE_R));
    CHECK(!haeun_pt_map_leaf(ram,sizeof(ram),l0,0u,0u,HAEUN_PTE_V|HAEUN_PTE_W));
    puts("all page-table helper tests passed");
    return 0;
}
