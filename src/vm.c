#include "haeun/vm.h"
#include "haeun/board_extra.h"
#include "haeun/board_mmio.h"
#include "haeun/board_net.h"
#include "haeun/cpu_state.h"
#include "haeun/exception_handler.h"
#include "haeun/instruction_decoder.h"
#include "haeun/instruction_executor.h"
#include "haeun/instruction_trace.h"
#include "haeun/memory_interface.h"

#include <string.h>

/**
 * Initializes a VM with the default ISA version.
 */
haeun_vm_result haeun_vm_init(haeun_vm *vm, uint8_t *ram, size_t ram_size,
                              uint32_t entry_pc) {
    return haeun_vm_init_version(vm, ram, ram_size, entry_pc, HAEUN_ISA_VERSION);
}

/**
 * Initializes CPU, memory binding, and optional empty device state.
 */
haeun_vm_result haeun_vm_init_version(haeun_vm *vm, uint8_t *ram, size_t ram_size,
                                      uint32_t entry_pc, uint32_t isa_version) {
    if (vm == NULL || ram == NULL || ram_size == 0u ||
        (isa_version < HAEUN_ISA_VERSION || isa_version > HAEUN_ISA_VERSION_4) ||
        (uint64_t)entry_pc + 4u > (uint64_t)ram_size)
        return HAEUN_VM_BAD_ARGUMENT;
    memset(vm, 0, sizeof(*vm));
    vm->ram = ram;
    vm->ram_size = ram_size;
    /* Default locally-administered unicast MAC. */
    vm->net_mac[0] = 0x02u;
    vm->net_mac[1] = 0x00u;
    vm->net_mac[2] = 0x00u;
    vm->net_mac[3] = 0x00u;
    vm->net_mac[4] = 0x00u;
    vm->net_mac[5] = 0x01u;
    haeun_cpu_reset(vm, entry_pc, isa_version);
    return HAEUN_VM_OK;
}

/**
 * Queues host UART RX bytes into the guest FIFO.
 */
bool haeun_vm_uart_feed(haeun_vm *vm, const uint8_t *bytes, size_t length) {
    if (vm == NULL || vm->uart_rx_count > HAEUN_UART_RX_CAPACITY ||
        (bytes == NULL && length != 0u) ||
        length > HAEUN_UART_RX_CAPACITY - vm->uart_rx_count)
        return false;
    for (size_t i = 0u; i < length; ++i) {
        size_t tail =
            (vm->uart_rx_head + vm->uart_rx_count) % HAEUN_UART_RX_CAPACITY;
        vm->uart_rx[tail] = bytes[i];
        ++vm->uart_rx_count;
    }
    haeun_board_uart_rx_event(vm);
    return true;
}

/**
 * Installs the host UART transmit callback.
 */
void haeun_vm_uart_set_tx_sink(haeun_vm *vm, haeun_uart_tx_sink sink,
                               void *context) {
    if (vm == NULL) return;
    vm->uart_tx = sink;
    vm->uart_tx_context = context;
}

/**
 * Attaches a raw block-device disk image.
 */
bool haeun_vm_block_attach(haeun_vm *vm, uint8_t *disk, size_t disk_size) {
    if (vm == NULL || disk == NULL || disk_size < HAEUN_BLOCK_SIZE ||
        disk_size % HAEUN_BLOCK_SIZE != 0u)
        return false;
    vm->block_disk = disk;
    vm->block_disk_size = disk_size;
    vm->block_status = 0u;
    vm->block_lba = 0u;
    vm->block_dma_address = 0u;
    vm->block_irq_enable = 0u;
    return true;
}

/**
 * Enables Boot ROM write protection for the firmware code window.
 */
void haeun_vm_protect_boot_rom(haeun_vm *vm) {
    if (vm == NULL) return;
    vm->boot_rom_size = HAEUN_BOOT_ROM_SIZE;
}

/**
 * Queues host keyboard scancodes into the guest FIFO.
 */
bool haeun_vm_keyboard_feed(haeun_vm *vm, const uint8_t *bytes, size_t length) {
    size_t i;
    if (vm == NULL || vm->kbd_rx_count > HAEUN_KBD_CAPACITY ||
        (bytes == NULL && length != 0u) ||
        length > HAEUN_KBD_CAPACITY - vm->kbd_rx_count)
        return false;
    for (i = 0u; i < length; ++i) {
        size_t tail =
            (vm->kbd_rx_head + vm->kbd_rx_count) % HAEUN_KBD_CAPACITY;
        vm->kbd_rx[tail] = bytes[i];
        ++vm->kbd_rx_count;
    }
    haeun_board_kbd_rx_event(vm);
    return true;
}

/**
 * Posts one mouse sample for the guest to read.
 */
bool haeun_vm_mouse_post(haeun_vm *vm, int32_t x, int32_t y, uint32_t buttons) {
    if (vm == NULL) return false;
    vm->mouse_x = x;
    vm->mouse_y = y;
    vm->mouse_buttons = buttons & 7u;
    vm->mouse_pending = 1u;
    haeun_board_mouse_event(vm);
    return true;
}

/**
 * Installs the host callback invoked when the guest kicks NIC TX.
 */
void haeun_vm_net_set_tx_sink(haeun_vm *vm, haeun_net_tx_sink sink,
                              void *context) {
    if (vm == NULL) return;
    vm->net_tx = sink;
    vm->net_tx_context = context;
}

/**
 * Installs the optional host ECALL I/O assist hook.
 */
void haeun_vm_set_ecall_io(haeun_vm *vm, haeun_ecall_io_fn fn, void *context) {
    if (vm == NULL) return;
    vm->ecall_io = fn;
    vm->ecall_io_context = context;
}

/**
 * Installs the optional host assist for fatal user-mode traps.
 */
void haeun_vm_set_fault_io(haeun_vm *vm, haeun_fault_io_fn fn, void *context) {
    if (vm == NULL)
        return;
    vm->fault_io = fn;
    vm->fault_io_context = context;
}

/**
 * Delivers a real Ethernet frame into the guest RX DMA buffer.
 */
bool haeun_vm_net_inject(haeun_vm *vm, const uint8_t *frame, uint32_t length) {
    uint64_t end;
    if (vm == NULL || frame == NULL || length < 42u ||
        length > HAEUN_NET_FRAME_MAX)
        return false;
    if ((vm->net_control & 1u) == 0u) return false;
    if (vm->net_rx_pending != 0u) return false;
    if (length > vm->net_rx_cap) return false;
    end = (uint64_t)vm->net_rx_addr + (uint64_t)length;
    if (end < (uint64_t)vm->net_rx_addr || end > (uint64_t)vm->ram_size)
        return false;
    memcpy(vm->ram + vm->net_rx_addr, frame, length);
    vm->net_rx_len = length;
    vm->net_rx_pending = 1u;
    haeun_board_net_rx_event(vm);
    return true;
}

/**
 * Single-steps one guest instruction (fetch → decode → execute → trace).
 */
haeun_vm_result haeun_vm_step(haeun_vm *vm) {
    uint32_t word = 0u, cause = 0u, tval = 0u, pc;
    haeun_decoded_insn decoded;
    haeun_vm_result result;
    if (vm == NULL || vm->ram == NULL) return HAEUN_VM_BAD_ARGUMENT;
    if (haeun_cpu_halted(vm)) return HAEUN_VM_HALTED;
    pc = haeun_cpu_pc(vm);
    if (!haeun_mem_fetch(vm, pc, &word, &cause, &tval))
        return haeun_exception_raise(
            vm,
            cause == 2u ? HAEUN_VM_ALIGNMENT_FAULT : HAEUN_VM_MEMORY_FAULT,
            cause, tval);
    if (!haeun_decode_instruction(word, vm->isa_version, &decoded, &cause))
        return haeun_exception_raise(vm, HAEUN_VM_ILLEGAL_INSTRUCTION, cause,
                                     0u);
    result = haeun_execute_instruction(vm, &decoded);
    haeun_trace_record(vm->trace, pc, word, decoded.op, haeun_cpu_pc(vm));
    return result;
}

/**
 * Runs until halt, fault, or the instruction budget is exhausted.
 */
haeun_vm_result haeun_vm_run(haeun_vm *vm, uint64_t max_steps,
                             uint64_t *steps_executed) {
    if (vm == NULL || steps_executed == NULL) return HAEUN_VM_BAD_ARGUMENT;
    *steps_executed = 0u;
    if (vm->ram == NULL) return HAEUN_VM_BAD_ARGUMENT;
    if (haeun_cpu_halted(vm)) return HAEUN_VM_HALTED;
    while (*steps_executed < max_steps) {
        haeun_vm_result result = haeun_vm_step(vm);
        if (result == HAEUN_VM_HALTED) {
            ++*steps_executed;
            return result;
        }
        if (result != HAEUN_VM_OK) return result;
        ++*steps_executed;
    }
    return HAEUN_VM_STEP_LIMIT;
}
