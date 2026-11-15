#include "haeun/guest_pcb.h"

enum {
    HAEUN_PCB_BASE = 0x0000A000u,
    HAEUN_PCB_CURR = 0x0000A400u,
    HAEUN_PCB_STRIDE = 0x100u,
    HAEUN_PCB_NPROC = HAEUN_GUEST_PCB_NPROC
};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/**
 * Loads PCB[curr].pid when the process-v1 table is present in RAM.
 */
uint32_t haeun_guest_pcb_current_pid(const haeun_vm *vm) {
    uint32_t curr;
    uint32_t off;
    if (vm == NULL || vm->ram == NULL)
        return 0u;
    if (vm->ram_size < (size_t)(HAEUN_PCB_CURR + 4u))
        return 0u;
    curr = rd32(vm->ram + HAEUN_PCB_CURR);
    if (curr >= HAEUN_PCB_NPROC)
        return 0u;
    off = HAEUN_PCB_BASE + curr * HAEUN_PCB_STRIDE + 4u;
    if ((size_t)off + 4u > vm->ram_size)
        return 0u;
    return rd32(vm->ram + off);
}
