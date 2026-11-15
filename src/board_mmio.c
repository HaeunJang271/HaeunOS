#include "haeun/board_mmio.h"
#include "haeun/board_extra.h"

#include <string.h>

enum {
    IRQ_TIMER = 0u,
    IRQ_UART = 1u,
    IRQ_BLOCK = 2u,
    IRQ_KBD = 3u,
    IRQ_MOUSE = 4u,
    IRQ_NET = 5u,
    IRQ_MASK = 0x3fu
};

static bool in_range(uint32_t address, uint32_t base) {
    return address >= base &&
           (uint64_t)address < (uint64_t)base + UINT32_C(0x1000);
}

bool haeun_board_maps(uint32_t address) {
    return in_range(address, HAEUN_IRQC_BASE) ||
           in_range(address, HAEUN_TIMER_BASE) ||
           in_range(address, HAEUN_UART_BASE) ||
           in_range(address, HAEUN_BLOCK_BASE) ||
           haeun_board_extra_maps(address);
}

void haeun_board_uart_rx_event(haeun_vm *vm) {
    if (vm == NULL) return;
    if ((vm->uart_control & 1u) != 0u && vm->uart_rx_count != 0u)
        vm->irq_pending |= (1u << IRQ_UART);
}

void haeun_board_block_done_event(haeun_vm *vm) {
    if (vm == NULL) return;
    if ((vm->block_irq_enable & 1u) != 0u)
        vm->irq_pending |= (1u << IRQ_BLOCK);
}

static void timer_fire_if_due(haeun_vm *vm) {
    if ((vm->timer_control & 1u) == 0u) return;
    if (vm->timer_count < vm->timer_compare) return;
    vm->irq_pending |= (1u << IRQ_TIMER);
    vm->timer_control &= ~1u;
}

void haeun_board_on_retire(haeun_vm *vm) {
    if (vm == NULL) return;
    vm->timer_count += 1u;
    timer_fire_if_due(vm);
}

uint32_t haeun_board_irq_claim(const haeun_vm *vm) {
    uint32_t ready;
    if (vm == NULL) return UINT32_C(0xffffffff);
    ready = vm->irq_pending & vm->irq_enable & IRQ_MASK;
    for (uint32_t id = 0u; id <= IRQ_NET; ++id) {
        if ((ready & (1u << id)) != 0u) return id;
    }
    return UINT32_C(0xffffffff);
}

static uint32_t uart_read(haeun_vm *vm, uint32_t offset) {
    if (offset == 0u) {
        if (vm->uart_rx_count == 0u) return UINT32_C(0x80000000);
        uint8_t byte = vm->uart_rx[vm->uart_rx_head];
        vm->uart_rx_head = (vm->uart_rx_head + 1u) % HAEUN_UART_RX_CAPACITY;
        --vm->uart_rx_count;
        return byte;
    }
    if (offset == 4u) return (vm->uart_rx_count != 0u ? 1u : 0u) | 2u;
    if (offset == 8u) return vm->uart_control;
    return 0u;
}

static void uart_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    if (offset == 0u) {
        if (vm->uart_tx != NULL) vm->uart_tx(vm->uart_tx_context, (uint8_t)value);
    } else if (offset == 8u) {
        vm->uart_control = value & 1u;
        haeun_board_uart_rx_event(vm);
    }
}

static uint32_t block_read(const haeun_vm *vm, uint32_t offset) {
    switch (offset) {
    case 0u: return vm->block_status;
    case 8u: return vm->block_lba;
    case 12u: return vm->block_dma_address;
    case 16u: return vm->block_irq_enable;
    default: return 0u;
    }
}

static void block_command(haeun_vm *vm, uint32_t command) {
    uint64_t disk_offset = (uint64_t)vm->block_lba * HAEUN_BLOCK_SIZE;
    uint64_t dma_end = (uint64_t)vm->block_dma_address + HAEUN_BLOCK_SIZE;
    vm->block_status = 1u;
    if (vm->block_disk == NULL || command < 1u || command > 2u ||
        (vm->block_dma_address & (HAEUN_BLOCK_SIZE - 1u)) != 0u ||
        disk_offset + HAEUN_BLOCK_SIZE > (uint64_t)vm->block_disk_size ||
        dma_end > (uint64_t)vm->ram_size) {
        vm->block_status = 3u;
        haeun_board_block_done_event(vm);
        return;
    }
    if (command == 1u)
        memmove(vm->ram + vm->block_dma_address,
                vm->block_disk + (size_t)disk_offset, HAEUN_BLOCK_SIZE);
    else
        memmove(vm->block_disk + (size_t)disk_offset,
                vm->ram + vm->block_dma_address, HAEUN_BLOCK_SIZE);
    vm->block_status = 2u;
    haeun_board_block_done_event(vm);
}

static void block_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    switch (offset) {
    case 4u: block_command(vm, value); break;
    case 8u: vm->block_lba = value; break;
    case 12u: vm->block_dma_address = value; break;
    case 16u: vm->block_irq_enable = value & 1u; break;
    default: break;
    }
}

static uint32_t timer_read(const haeun_vm *vm, uint32_t offset) {
    switch (offset) {
    case 0u: return (uint32_t)vm->timer_count;
    case 4u: return (uint32_t)(vm->timer_count >> 32);
    case 8u: return (uint32_t)vm->timer_compare;
    case 12u: return (uint32_t)(vm->timer_compare >> 32);
    case 16u: return vm->timer_control;
    default: return 0u;
    }
}

static void timer_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    switch (offset) {
    case 8u:
        vm->timer_compare = (vm->timer_compare & ~UINT64_C(0xffffffff)) | value;
        break;
    case 12u:
        vm->timer_compare = (vm->timer_compare & UINT64_C(0xffffffff)) |
                            ((uint64_t)value << 32);
        break;
    case 16u:
        vm->timer_control = value & 1u;
        timer_fire_if_due(vm);
        break;
    default:
        break;
    }
}

static uint32_t irqc_read(const haeun_vm *vm, uint32_t offset) {
    switch (offset) {
    case 0u: return vm->irq_pending & IRQ_MASK;
    case 4u: return vm->irq_enable & IRQ_MASK;
    case 8u: return haeun_board_irq_claim(vm);
    default: return 0u;
    }
}

static void irqc_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    if (offset == 4u) {
        vm->irq_enable = value & IRQ_MASK;
    } else if (offset == 12u && value <= IRQ_NET) {
        vm->irq_pending &= ~(1u << value);
    }
}

uint32_t haeun_board_read(haeun_vm *vm, uint32_t address) {
    if (in_range(address, HAEUN_IRQC_BASE))
        return irqc_read(vm, address - HAEUN_IRQC_BASE);
    if (in_range(address, HAEUN_TIMER_BASE))
        return timer_read(vm, address - HAEUN_TIMER_BASE);
    if (in_range(address, HAEUN_UART_BASE))
        return uart_read(vm, address - HAEUN_UART_BASE);
    if (in_range(address, HAEUN_BLOCK_BASE))
        return block_read(vm, address - HAEUN_BLOCK_BASE);
    return haeun_board_extra_read(vm, address);
}

void haeun_board_write(haeun_vm *vm, uint32_t address, uint32_t value) {
    if (in_range(address, HAEUN_IRQC_BASE))
        irqc_write(vm, address - HAEUN_IRQC_BASE, value);
    else if (in_range(address, HAEUN_TIMER_BASE))
        timer_write(vm, address - HAEUN_TIMER_BASE, value);
    else if (in_range(address, HAEUN_UART_BASE))
        uart_write(vm, address - HAEUN_UART_BASE, value);
    else if (in_range(address, HAEUN_BLOCK_BASE))
        block_write(vm, address - HAEUN_BLOCK_BASE, value);
    else
        haeun_board_extra_write(vm, address, value);
}
