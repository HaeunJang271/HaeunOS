#ifndef HAEUN_VM_H
#define HAEUN_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HAEUN_ISA_VERSION 1u
#define HAEUN_ISA_VERSION_2 2u
#define HAEUN_ISA_VERSION_3 3u
#define HAEUN_ISA_VERSION_4 4u
#define HAEUN_PRIV_USER 0u
#define HAEUN_PRIV_SUPERVISOR 1u
#define HAEUN_REGISTER_COUNT 16u
#define HAEUN_PAGE_SIZE 4096u
/** Write-protected Boot ROM code window; DMA scratch begins at 0x8000. */
#define HAEUN_BOOT_ROM_SIZE UINT32_C(0x8000)
#define HAEUN_IRQC_BASE UINT32_C(0x10000000)
#define HAEUN_TIMER_BASE UINT32_C(0x10001000)
#define HAEUN_UART_BASE UINT32_C(0x10002000)
#define HAEUN_UART_APERTURE_SIZE UINT32_C(0x1000)
#define HAEUN_UART_RX_CAPACITY 4096u
#define HAEUN_BLOCK_BASE UINT32_C(0x10003000)
#define HAEUN_BLOCK_APERTURE_SIZE UINT32_C(0x1000)
#define HAEUN_BLOCK_SIZE 512u
#define HAEUN_FB_BASE UINT32_C(0x10004000)
#define HAEUN_KBD_BASE UINT32_C(0x10005000)
#define HAEUN_MOUSE_BASE UINT32_C(0x10006000)
#define HAEUN_NET_BASE UINT32_C(0x10007000)
#define HAEUN_KBD_CAPACITY 64u
#define HAEUN_NET_FRAME_MAX 1518u
#define HAEUN_TLB_ENTRIES 16u

typedef struct haeun_instruction_trace haeun_instruction_trace;

typedef struct haeun_vm haeun_vm;

typedef void (*haeun_uart_tx_sink)(void *context, uint8_t byte);
typedef void (*haeun_net_tx_sink)(void *context, const uint8_t *frame,
                                  uint32_t length);
/** Optional host ECALL I/O assist; true means the call was completed (no trap). */
typedef bool (*haeun_ecall_io_fn)(haeun_vm *vm, void *context);
/**
 * Optional host user-fault assist (page fault / illegal / privilege, etc.).
 * Invoked from trap enter before the guest handler runs.
 */
typedef void (*haeun_fault_io_fn)(haeun_vm *vm, void *context, uint32_t cause,
                                  uint32_t tval, uint32_t epc);

typedef enum {
    HAEUN_VM_OK = 0,
    HAEUN_VM_HALTED,
    HAEUN_VM_TRAP,
    HAEUN_VM_ILLEGAL_INSTRUCTION,
    HAEUN_VM_MEMORY_FAULT,
    HAEUN_VM_ALIGNMENT_FAULT,
    HAEUN_VM_STEP_LIMIT,
    HAEUN_VM_BAD_ARGUMENT
} haeun_vm_result;

struct haeun_vm {
    uint32_t r[HAEUN_REGISTER_COUNT];
    uint32_t pc;
    uint32_t isa_version;
    uint32_t privilege;
    uint32_t cause;
    uint32_t fault_pc;
    uint32_t csr_status;
    uint32_t csr_trapvec;
    uint32_t csr_epc;
    uint32_t csr_cause;
    uint32_t csr_tval;
    uint32_t csr_scratch;
    uint32_t csr_ptbr;
    uint8_t *ram;
    size_t ram_size;
    uint8_t uart_rx[HAEUN_UART_RX_CAPACITY];
    size_t uart_rx_head;
    size_t uart_rx_count;
    uint32_t uart_control;
    haeun_uart_tx_sink uart_tx;
    void *uart_tx_context;
    uint8_t *block_disk;
    size_t block_disk_size;
    uint32_t block_status;
    uint32_t block_lba;
    uint32_t block_dma_address;
    uint32_t block_irq_enable;
    uint64_t timer_count;
    uint64_t timer_compare;
    uint32_t timer_control;
    uint32_t irq_pending;
    uint32_t irq_enable;
    bool halted;
    uint32_t tlb_vpn[HAEUN_TLB_ENTRIES];
    uint32_t tlb_pte[HAEUN_TLB_ENTRIES];
    bool tlb_valid[HAEUN_TLB_ENTRIES];
    uint32_t tlb_next;
    /** Optional single-step trace sink; NULL disables recording. */
    haeun_instruction_trace *trace;
    /** Bytes at PA 0 write-protected as Boot ROM; 0 disables. */
    uint32_t boot_rom_size;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_base;
    uint32_t fb_control;
    uint8_t kbd_rx[HAEUN_KBD_CAPACITY];
    size_t kbd_rx_head;
    size_t kbd_rx_count;
    uint32_t kbd_control;
    int32_t mouse_x;
    int32_t mouse_y;
    uint32_t mouse_buttons;
    uint32_t mouse_pending;
    uint32_t mouse_control;
    uint8_t net_mac[6];
    uint32_t net_control;
    uint32_t net_tx_addr;
    uint32_t net_tx_len;
    uint32_t net_rx_addr;
    uint32_t net_rx_cap;
    uint32_t net_rx_len;
    uint32_t net_rx_pending;
    haeun_net_tx_sink net_tx;
    void *net_tx_context;
    haeun_ecall_io_fn ecall_io;
    void *ecall_io_context;
    haeun_fault_io_fn fault_io;
    void *fault_io_context;
};

/** Initializes a VM over caller-owned RAM. RAM size must be nonzero. */
haeun_vm_result haeun_vm_init(haeun_vm *vm, uint8_t *ram, size_t ram_size,
                              uint32_t entry_pc);
/** Selects an explicitly versioned guest ISA; supported versions are 1 through 4. */
haeun_vm_result haeun_vm_init_version(haeun_vm *vm, uint8_t *ram, size_t ram_size,
                                      uint32_t entry_pc, uint32_t isa_version);
/** Atomically queues host input bytes; false means invalid arguments or no room. */
bool haeun_vm_uart_feed(haeun_vm *vm, const uint8_t *bytes, size_t length);
/** Installs the host output sink. With no sink, guest TX bytes are discarded. */
void haeun_vm_uart_set_tx_sink(haeun_vm *vm, haeun_uart_tx_sink sink, void *context);
/** Attaches caller-owned raw disk bytes (one or more 512-byte blocks). */
bool haeun_vm_block_attach(haeun_vm *vm, uint8_t *disk, size_t disk_size);
/** Write-protects Boot ROM code (`0x0000–0x7FFF`) after HFW staging. */
void haeun_vm_protect_boot_rom(haeun_vm *vm);
/** Queues keyboard scancodes; false on overflow or bad arguments. */
bool haeun_vm_keyboard_feed(haeun_vm *vm, const uint8_t *bytes, size_t length);
/** Posts one mouse sample and optionally latches IRQC source 4. */
bool haeun_vm_mouse_post(haeun_vm *vm, int32_t x, int32_t y, uint32_t buttons);
/** Installs the host NIC transmit sink (receives guest DMA frame bytes). */
void haeun_vm_net_set_tx_sink(haeun_vm *vm, haeun_net_tx_sink sink,
                              void *context);
/**
 * Copies a real Ethernet frame into the guest RX DMA buffer when the NIC
 * is enabled and RX_ADDR/RX_CAP can hold it.
 */
bool haeun_vm_net_inject(haeun_vm *vm, const uint8_t *frame, uint32_t length);
/**
 * Installs an optional host ECALL I/O assist (process-HFS path OPEN, etc.).
 */
void haeun_vm_set_ecall_io(haeun_vm *vm, haeun_ecall_io_fn fn, void *context);
/**
 * Installs an optional host assist for fatal user-mode traps (cleanup/log).
 */
void haeun_vm_set_fault_io(haeun_vm *vm, haeun_fault_io_fn fn, void *context);
/** Executes exactly one instruction; PC advances only as specified by ISA. */
haeun_vm_result haeun_vm_step(haeun_vm *vm);
/** Executes at most max_steps instructions and returns the first stop reason. */
haeun_vm_result haeun_vm_run(haeun_vm *vm, uint64_t max_steps,
                             uint64_t *steps_executed);

#endif
