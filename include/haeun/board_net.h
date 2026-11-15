#ifndef HAEUN_BOARD_NET_H
#define HAEUN_BOARD_NET_H

#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

/** True when address is in the NIC aperture. */
bool haeun_board_net_maps(uint32_t address);

/** 32-bit NIC MMIO load. */
uint32_t haeun_board_net_read(haeun_vm *vm, uint32_t address);

/** 32-bit NIC MMIO store (may kick TX). */
void haeun_board_net_write(haeun_vm *vm, uint32_t address, uint32_t value);

/** Latches NIC RX IRQ when CONTROL enables it and a frame is pending. */
void haeun_board_net_rx_event(haeun_vm *vm);

#endif
