#include "haeun/board_net.h"

#include <stdio.h>
#include <string.h>

enum { IRQ_NET = 5u };

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-nic] security/validation failure: %s\n", what);
}

static bool in_range(uint32_t address) {
    return address >= HAEUN_NET_BASE &&
           (uint64_t)address < (uint64_t)HAEUN_NET_BASE + UINT32_C(0x1000);
}

/**
 * Reports whether the address hits the virtual NIC aperture.
 */
bool haeun_board_net_maps(uint32_t address) { return in_range(address); }

/**
 * Latches IRQC source 5 when RX IRQ is enabled and a frame is pending.
 */
void haeun_board_net_rx_event(haeun_vm *vm) {
    if (vm == NULL) return;
    if ((vm->net_control & 2u) != 0u && vm->net_rx_pending != 0u)
        vm->irq_pending |= (1u << IRQ_NET);
}

static int dma_ok(const haeun_vm *vm, uint32_t addr, uint32_t len) {
    uint64_t end;
    if (len == 0u || len > HAEUN_NET_FRAME_MAX) return 0;
    end = (uint64_t)addr + (uint64_t)len;
    return end >= (uint64_t)addr && end <= (uint64_t)vm->ram_size;
}

static void do_tx(haeun_vm *vm) {
    if ((vm->net_control & 1u) == 0u) {
        log_fail("tx while disabled");
        return;
    }
    if (!dma_ok(vm, vm->net_tx_addr, vm->net_tx_len)) {
        log_fail("tx DMA out of range");
        return;
    }
    if (vm->net_tx != NULL)
        vm->net_tx(vm->net_tx_context, vm->ram + vm->net_tx_addr,
                   vm->net_tx_len);
}

/**
 * Reads a NIC register.
 */
uint32_t haeun_board_net_read(haeun_vm *vm, uint32_t address) {
    uint32_t off;
    if (vm == NULL || !in_range(address)) return 0u;
    off = address - HAEUN_NET_BASE;
    switch (off) {
    case 0u:
        return vm->net_rx_pending != 0u ? 1u : 0u;
    case 4u:
        return vm->net_control & 3u;
    case 8u:
        return (uint32_t)vm->net_mac[0] | ((uint32_t)vm->net_mac[1] << 8) |
               ((uint32_t)vm->net_mac[2] << 16) |
               ((uint32_t)vm->net_mac[3] << 24);
    case 12u:
        return (uint32_t)vm->net_mac[4] | ((uint32_t)vm->net_mac[5] << 8);
    case 16u:
        return vm->net_tx_addr;
    case 20u:
        return vm->net_tx_len;
    case 28u:
        return vm->net_rx_addr;
    case 32u:
        return vm->net_rx_cap;
    case 36u:
        return vm->net_rx_len;
    default:
        return 0u;
    }
}

/**
 * Writes a NIC register; TX_CMD and RX_ACK have side effects.
 */
void haeun_board_net_write(haeun_vm *vm, uint32_t address, uint32_t value) {
    uint32_t off;
    if (vm == NULL || !in_range(address)) return;
    off = address - HAEUN_NET_BASE;
    switch (off) {
    case 4u:
        vm->net_control = value & 3u;
        break;
    case 8u:
        vm->net_mac[0] = (uint8_t)value;
        vm->net_mac[1] = (uint8_t)(value >> 8);
        vm->net_mac[2] = (uint8_t)(value >> 16);
        vm->net_mac[3] = (uint8_t)(value >> 24);
        break;
    case 12u:
        vm->net_mac[4] = (uint8_t)value;
        vm->net_mac[5] = (uint8_t)(value >> 8);
        break;
    case 16u:
        vm->net_tx_addr = value;
        break;
    case 20u:
        vm->net_tx_len = value;
        break;
    case 24u:
        if (value == 1u) do_tx(vm);
        break;
    case 28u:
        vm->net_rx_addr = value;
        break;
    case 32u:
        vm->net_rx_cap = value;
        break;
    case 40u:
        if (value == 1u) vm->net_rx_pending = 0u;
        break;
    default:
        break;
    }
}
