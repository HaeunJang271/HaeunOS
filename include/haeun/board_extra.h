#ifndef HAEUN_BOARD_EXTRA_H
#define HAEUN_BOARD_EXTRA_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/** True when address is framebuffer, keyboard, mouse, or NIC MMIO. */
bool haeun_board_extra_maps(uint32_t address);

/** 32-bit MMIO load from an extra board aperture. */
uint32_t haeun_board_extra_read(haeun_vm *vm, uint32_t address);

/** 32-bit MMIO store to an extra board aperture. */
void haeun_board_extra_write(haeun_vm *vm, uint32_t address, uint32_t value);

/** Latches keyboard IRQC pending when enabled and FIFO nonempty. */
void haeun_board_kbd_rx_event(haeun_vm *vm);

/** Latches mouse IRQC pending when enabled and a sample is waiting. */
void haeun_board_mouse_event(haeun_vm *vm);

#endif
