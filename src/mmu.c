#include "haeun/mmu.h"
#include "haeun/tlb.h"

enum {
    PTE_V = 1u,
    PTE_R = 2u,
    PTE_W = 4u,
    PTE_X = 8u,
    PTE_U = 16u,
    PTE_RES_MASK = 0xfe0u,
    PTBR_MODE = 1u
};

static bool phys_in_ram(const haeun_vm *vm, uint32_t pa, uint32_t size) {
    return (uint64_t)pa + (uint64_t)size <= (uint64_t)vm->ram_size;
}

static uint32_t load_phys(const haeun_vm *vm, uint32_t pa) {
    const uint8_t *p = vm->ram + pa;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool pte_reserved_clear(uint32_t pte) {
    return (pte & PTE_RES_MASK) == 0u;
}

static bool check_leaf(haeun_vm *vm, uint32_t pte, uint32_t access, uint32_t va,
                       uint32_t *pa, uint32_t *cause) {
    uint32_t need;
    if (!pte_reserved_clear(pte) || (pte & (PTE_R | PTE_W | PTE_X)) == 0u ||
        ((pte & PTE_W) != 0u && (pte & PTE_R) == 0u)) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    if (vm->privilege == HAEUN_PRIV_USER && (pte & PTE_U) == 0u) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    need = 0u;
    if ((access & HAEUN_MMU_ACCESS_R) != 0u) need |= PTE_R;
    if ((access & HAEUN_MMU_ACCESS_W) != 0u) need |= PTE_W;
    if ((access & HAEUN_MMU_ACCESS_X) != 0u) need |= PTE_X;
    if ((pte & need) != need) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    *pa = ((pte >> 12) << 12) | (va & 0xfffu);
    return true;
}

/**
 * Translates va for access, using the TLB on hits and filling it after walks.
 */
bool haeun_mmu_translate(haeun_vm *vm, uint32_t va, uint32_t access,
                         uint32_t *pa, uint32_t *cause) {
    uint32_t root, l1_pa, pte1, l0_pa, pte0, vpn;
    if (vm == NULL || pa == NULL || cause == NULL) {
        if (cause != NULL) *cause = 1u;
        return false;
    }
    if ((vm->csr_ptbr & PTBR_MODE) == 0u) {
        *pa = va;
        return true;
    }
    vpn = va >> 12;
    if (haeun_tlb_lookup(vm, vpn, &pte0)) {
        return check_leaf(vm, pte0, access, va, pa, cause);
    }
    root = vm->csr_ptbr & ~0xfffu;
    l1_pa = root + ((va >> 22) & 0x3ffu) * 4u;
    if (!phys_in_ram(vm, l1_pa, 4u)) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    pte1 = load_phys(vm, l1_pa);
    if ((pte1 & PTE_V) == 0u || !pte_reserved_clear(pte1)) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    if ((pte1 & (PTE_R | PTE_W | PTE_X)) != 0u) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    l0_pa = ((pte1 >> 12) << 12) + ((va >> 12) & 0x3ffu) * 4u;
    if (!phys_in_ram(vm, l0_pa, 4u)) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    pte0 = load_phys(vm, l0_pa);
    if ((pte0 & PTE_V) == 0u) {
        *cause = HAEUN_CAUSE_PAGE_FAULT;
        return false;
    }
    if (!check_leaf(vm, pte0, access, va, pa, cause)) return false;
    haeun_tlb_insert(vm, vpn, pte0);
    return true;
}
