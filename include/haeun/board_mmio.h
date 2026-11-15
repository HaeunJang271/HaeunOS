#ifndef HAEUN_BOARD_MMIO_H
#define HAEUN_BOARD_MMIO_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Returns true when the guest physical address lies in an implemented
 * board MMIO aperture (IRQC, timer, UART, block, FB, keyboard, mouse).
 */
bool haeun_board_maps(uint32_t address);

/**
 * Performs a 32-bit aligned MMIO load from an implemented board aperture.
 */
uint32_t haeun_board_read(haeun_vm *vm, uint32_t address);

/**
 * Performs a 32-bit aligned MMIO store to an implemented board aperture.
 */
void haeun_board_write(haeun_vm *vm, uint32_t address, uint32_t value);

/**
 * Advances the virtual timer by one tick after a retired instruction and
 * may latch the timer IRQC pending bit.
 */
void haeun_board_on_retire(haeun_vm *vm);

/**
 * Latches UART RX pending when RX interrupt enable is set and the FIFO
 * is nonempty.
 */
void haeun_board_uart_rx_event(haeun_vm *vm);

/**
 * Latches block completion pending when block IRQ_ENABLE is set.
 */
void haeun_board_block_done_event(haeun_vm *vm);

/**
 * Returns the lowest IRQC source that is both pending and enabled, or
 * 0xFFFFFFFF when none are ready.
 */
uint32_t haeun_board_irq_claim(const haeun_vm *vm);

#endif
