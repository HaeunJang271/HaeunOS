#ifndef HAEUN_PT_H
#define HAEUN_PT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HAEUN_PTE_V UINT32_C(1)
#define HAEUN_PTE_R UINT32_C(2)
#define HAEUN_PTE_W UINT32_C(4)
#define HAEUN_PTE_X UINT32_C(8)
#define HAEUN_PTE_U UINT32_C(16)
#define HAEUN_PTBR_MODE UINT32_C(1)

/**
 * Writes a non-leaf L1 PTE at vpn1 that points to the L0 table page at
 * l0_pa. Both addresses must be 4 KiB aligned and lie in ram.
 */
bool haeun_pt_set_nonleaf(uint8_t *ram, size_t ram_size, uint32_t l1_pa,
                          uint32_t vpn1, uint32_t l0_pa);

/**
 * Writes a leaf L0 PTE mapping vpn0 to ppn with the given flag bits
 * (must include V and a valid R/W/X combination).
 */
bool haeun_pt_map_leaf(uint8_t *ram, size_t ram_size, uint32_t l0_pa,
                       uint32_t vpn0, uint32_t ppn, uint32_t flags);

/**
 * Maps one 4 KiB page va->pa using L1 at l1_pa and L0 at l0_pa for the
 * VA's VPN1. Installs the non-leaf entry when needed.
 */
bool haeun_pt_map_page(uint8_t *ram, size_t ram_size, uint32_t l1_pa,
                       uint32_t l0_pa, uint32_t va, uint32_t pa,
                       uint32_t flags);

/** Returns a ptbr word with root PPN from l1_pa and MODE enabled. */
uint32_t haeun_pt_make_ptbr(uint32_t l1_pa);

#endif
