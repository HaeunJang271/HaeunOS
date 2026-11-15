#include "haeun/board_extra.h"
#include "haeun/board_net.h"

enum {
    IRQ_KBD = 3u,
    IRQ_MOUSE = 4u
};

static bool in_range(uint32_t address, uint32_t base) {
    return address >= base &&
           (uint64_t)address < (uint64_t)base + UINT32_C(0x1000);
}

/**
 * Reports whether the address hits FB/keyboard/mouse apertures.
 */
bool haeun_board_extra_maps(uint32_t address) {
    return in_range(address, HAEUN_FB_BASE) ||
           in_range(address, HAEUN_KBD_BASE) ||
           in_range(address, HAEUN_MOUSE_BASE) ||
           haeun_board_net_maps(address);
}

/**
 * Latches keyboard IRQ when CONTROL enables RX and the FIFO has data.
 */
void haeun_board_kbd_rx_event(haeun_vm *vm) {
    if (vm == NULL) return;
    if ((vm->kbd_control & 1u) != 0u && vm->kbd_rx_count != 0u)
        vm->irq_pending |= (1u << IRQ_KBD);
}

/**
 * Latches mouse IRQ when CONTROL enables it and a sample is pending.
 */
void haeun_board_mouse_event(haeun_vm *vm) {
    if (vm == NULL) return;
    if ((vm->mouse_control & 1u) != 0u && vm->mouse_pending != 0u)
        vm->irq_pending |= (1u << IRQ_MOUSE);
}

static bool fb_range_ok(const haeun_vm *vm) {
    uint64_t bytes, end;
    if (vm->fb_width == 0u || vm->fb_width > 1024u ||
        vm->fb_height == 0u || vm->fb_height > 1024u)
        return false;
    bytes = (uint64_t)vm->fb_width * (uint64_t)vm->fb_height * 4u;
    end = (uint64_t)vm->fb_base + bytes;
    return end <= (uint64_t)vm->ram_size;
}

static uint32_t fb_read(const haeun_vm *vm, uint32_t offset) {
    switch (offset) {
    case 0u:
        return ((vm->fb_control & 1u) != 0u && fb_range_ok(vm)) ? 1u : 0u;
    case 4u:
        return vm->fb_control & 1u;
    case 8u:
        return vm->fb_width;
    case 12u:
        return vm->fb_height;
    case 16u:
        return vm->fb_base;
    case 20u:
        return 1u;
    default:
        return 0u;
    }
}

static void fb_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    switch (offset) {
    case 4u:
        vm->fb_control = value & 1u;
        break;
    case 8u:
        vm->fb_width = value;
        break;
    case 12u:
        vm->fb_height = value;
        break;
    case 16u:
        vm->fb_base = value;
        break;
    default:
        break;
    }
}

static uint32_t kbd_read(haeun_vm *vm, uint32_t offset) {
    if (offset == 0u) {
        uint8_t byte;
        if (vm->kbd_rx_count == 0u) return UINT32_C(0x80000000);
        byte = vm->kbd_rx[vm->kbd_rx_head];
        vm->kbd_rx_head = (vm->kbd_rx_head + 1u) % HAEUN_KBD_CAPACITY;
        --vm->kbd_rx_count;
        return byte;
    }
    if (offset == 4u) return vm->kbd_rx_count != 0u ? 1u : 0u;
    if (offset == 8u) return vm->kbd_control & 1u;
    return 0u;
}

static void kbd_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    if (offset == 8u) {
        vm->kbd_control = value & 1u;
        haeun_board_kbd_rx_event(vm);
    }
}

static uint32_t mouse_read(const haeun_vm *vm, uint32_t offset) {
    switch (offset) {
    case 0u:
        return vm->mouse_pending != 0u ? 1u : 0u;
    case 4u:
        return (uint32_t)vm->mouse_x;
    case 8u:
        return (uint32_t)vm->mouse_y;
    case 12u:
        return vm->mouse_buttons & 7u;
    case 16u:
        return vm->mouse_control & 1u;
    default:
        return 0u;
    }
}

static void mouse_write(haeun_vm *vm, uint32_t offset, uint32_t value) {
    if (offset == 16u) {
        vm->mouse_control = value & 1u;
        haeun_board_mouse_event(vm);
    } else if (offset == 20u) {
        vm->mouse_pending = 0u;
    }
}

/**
 * Dispatches an extra-device MMIO load.
 */
uint32_t haeun_board_extra_read(haeun_vm *vm, uint32_t address) {
    if (in_range(address, HAEUN_FB_BASE))
        return fb_read(vm, address - HAEUN_FB_BASE);
    if (in_range(address, HAEUN_KBD_BASE))
        return kbd_read(vm, address - HAEUN_KBD_BASE);
    if (in_range(address, HAEUN_MOUSE_BASE))
        return mouse_read(vm, address - HAEUN_MOUSE_BASE);
    return haeun_board_net_read(vm, address);
}

/**
 * Dispatches an extra-device MMIO store.
 */
void haeun_board_extra_write(haeun_vm *vm, uint32_t address, uint32_t value) {
    if (in_range(address, HAEUN_FB_BASE))
        fb_write(vm, address - HAEUN_FB_BASE, value);
    else if (in_range(address, HAEUN_KBD_BASE))
        kbd_write(vm, address - HAEUN_KBD_BASE, value);
    else if (in_range(address, HAEUN_MOUSE_BASE))
        mouse_write(vm, address - HAEUN_MOUSE_BASE, value);
    else
        haeun_board_net_write(vm, address, value);
}
