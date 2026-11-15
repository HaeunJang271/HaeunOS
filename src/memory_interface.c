#include "haeun/memory_interface.h"
#include "haeun/board_mmio.h"
#include "haeun/mmu.h"

static uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static bool translate_access(haeun_vm *vm, uint32_t va, uint32_t access,
                             uint32_t *pa, uint32_t *cause, uint32_t *tval) {
    if (!haeun_mmu_translate(vm, va, access, pa, cause)) {
        *tval = va;
        return false;
    }
    return true;
}

/**
 * Fetches a little-endian instruction word through the MMU.
 */
bool haeun_mem_fetch(haeun_vm *vm, uint32_t va, uint32_t *word,
                     uint32_t *cause, uint32_t *tval) {
    uint32_t pa = 0u;
    if (vm == NULL || word == NULL || cause == NULL || tval == NULL) return false;
    *tval = va;
    if ((va & 3u) != 0u) {
        *cause = 2u;
        return false;
    }
    if (!translate_access(vm, va, HAEUN_MMU_ACCESS_X, &pa, cause, tval))
        return false;
    if ((uint64_t)pa + 4u > (uint64_t)vm->ram_size) {
        *cause = 1u;
        return false;
    }
    *word = load_le32(vm->ram + pa);
    return true;
}

/**
 * Loads an aligned 32-bit value from guest virtual memory or MMIO.
 */
bool haeun_mem_load_u32(haeun_vm *vm, uint32_t va, uint32_t *value,
                        uint32_t *cause, uint32_t *tval) {
    uint32_t pa = 0u;
    if (vm == NULL || value == NULL || cause == NULL || tval == NULL) return false;
    *tval = va;
    if ((va & 3u) != 0u) {
        *cause = 2u;
        return false;
    }
    if (!translate_access(vm, va, HAEUN_MMU_ACCESS_R, &pa, cause, tval))
        return false;
    if ((uint64_t)pa + 4u <= (uint64_t)vm->ram_size) {
        *value = load_le32(vm->ram + pa);
        return true;
    }
    if (haeun_board_maps(pa)) {
        *value = haeun_board_read(vm, pa);
        return true;
    }
    *cause = 1u;
    return false;
}

/**
 * Stores an aligned 32-bit value to guest virtual memory or MMIO.
 */
bool haeun_mem_store_u32(haeun_vm *vm, uint32_t va, uint32_t value,
                         uint32_t *cause, uint32_t *tval) {
    uint32_t pa = 0u;
    if (vm == NULL || cause == NULL || tval == NULL) return false;
    *tval = va;
    if ((va & 3u) != 0u) {
        *cause = 2u;
        return false;
    }
    if (!translate_access(vm, va, HAEUN_MMU_ACCESS_W, &pa, cause, tval))
        return false;
    if (vm->boot_rom_size != 0u && pa < vm->boot_rom_size) {
        *cause = 1u;
        return false;
    }
    if ((uint64_t)pa + 4u <= (uint64_t)vm->ram_size) {
        store_le32(vm->ram + pa, value);
        return true;
    }
    if (haeun_board_maps(pa)) {
        haeun_board_write(vm, pa, value);
        return true;
    }
    *cause = 1u;
    return false;
}
