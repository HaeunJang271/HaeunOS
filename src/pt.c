#include "haeun/pt.h"

#include <string.h>

static void store_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool page_in_ram(size_t ram_size, uint32_t pa) {
    return (pa & 0xfffu) == 0u &&
           (uint64_t)pa + 4096u <= (uint64_t)ram_size;
}

static bool leaf_flags_ok(uint32_t flags) {
    if ((flags & HAEUN_PTE_V) == 0u) return false;
    if ((flags & (HAEUN_PTE_R | HAEUN_PTE_W | HAEUN_PTE_X)) == 0u) return false;
    if ((flags & HAEUN_PTE_W) != 0u && (flags & HAEUN_PTE_R) == 0u) return false;
    if ((flags & ~UINT32_C(0x1f)) != 0u) return false;
    return true;
}

bool haeun_pt_set_nonleaf(uint8_t *ram, size_t ram_size, uint32_t l1_pa,
                          uint32_t vpn1, uint32_t l0_pa) {
    uint32_t slot;
    if (ram == NULL || !page_in_ram(ram_size, l1_pa) ||
        !page_in_ram(ram_size, l0_pa) || vpn1 > 1023u)
        return false;
    slot = l1_pa + vpn1 * 4u;
    store_le32(ram + slot, ((l0_pa >> 12) << 12) | HAEUN_PTE_V);
    return true;
}

bool haeun_pt_map_leaf(uint8_t *ram, size_t ram_size, uint32_t l0_pa,
                       uint32_t vpn0, uint32_t ppn, uint32_t flags) {
    uint32_t slot;
    if (ram == NULL || !page_in_ram(ram_size, l0_pa) || vpn0 > 1023u ||
        ppn > 0xfffffu || !leaf_flags_ok(flags))
        return false;
    slot = l0_pa + vpn0 * 4u;
    store_le32(ram + slot, (ppn << 12) | flags);
    return true;
}

bool haeun_pt_map_page(uint8_t *ram, size_t ram_size, uint32_t l1_pa,
                       uint32_t l0_pa, uint32_t va, uint32_t pa,
                       uint32_t flags) {
    uint32_t vpn1, vpn0, existing;
    if ((va & 0xfffu) != 0u || (pa & 0xfffu) != 0u) return false;
    vpn1 = (va >> 22) & 0x3ffu;
    vpn0 = (va >> 12) & 0x3ffu;
    if (!haeun_pt_set_nonleaf(ram, ram_size, l1_pa, vpn1, l0_pa)) return false;
    existing = load_le32(ram + l1_pa + vpn1 * 4u);
    if ((existing & ~0xfffu) != (l0_pa & ~0xfffu)) return false;
    return haeun_pt_map_leaf(ram, ram_size, l0_pa, vpn0, pa >> 12, flags);
}

uint32_t haeun_pt_make_ptbr(uint32_t l1_pa) {
    return (l1_pa & ~0xfffu) | HAEUN_PTBR_MODE;
}
