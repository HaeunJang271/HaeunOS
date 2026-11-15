#include "haeun/board_mmio.h"
#include "haeun/memory_interface.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))

static void put_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

/**
 * Boot ROM code window rejects stores after protect; scratch above 0x8000 works.
 */
static int test_boot_rom_protect(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    uint32_t cause=0u, tval=0u;
    memset(ram, 0, sizeof(ram));
    put_le32(ram, ENC(1, 0, 0, 0, 0)); /* HALT in ROM */
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_protect_boot_rom(&vm);
    CHECK(!haeun_mem_store_u32(&vm, 0u, 0x12345678u, &cause, &tval));
    CHECK(cause == 1u);
    CHECK(haeun_mem_store_u32(&vm, 0x8000u, 0xAAu, &cause, &tval));
    CHECK(ram[0x8000] == 0xAAu);
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_HALTED);
    return 0;
}

/**
 * Framebuffer STATUS reflects a valid BASE/size configuration.
 */
static int test_framebuffer_config(void) {
    uint8_t ram[4096];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_board_maps(HAEUN_FB_BASE));
    haeun_board_write(&vm, HAEUN_FB_BASE + 8u, 16u);
    haeun_board_write(&vm, HAEUN_FB_BASE + 12u, 16u);
    haeun_board_write(&vm, HAEUN_FB_BASE + 16u, 0u);
    haeun_board_write(&vm, HAEUN_FB_BASE + 4u, 1u);
    CHECK(haeun_board_read(&vm, HAEUN_FB_BASE) == 1u);
    CHECK(haeun_board_read(&vm, HAEUN_FB_BASE + 20u) == 1u);
    return 0;
}

/**
 * Keyboard FIFO and mouse sample path.
 */
static int test_keyboard_mouse(void) {
    uint8_t ram[256];
    haeun_vm vm;
    const uint8_t keys[] = {0x1Cu, 0x1Du};
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_vm_keyboard_feed(&vm, keys, 2u));
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE + 4u) == 1u);
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE) == 0x1Cu);
    CHECK(haeun_vm_mouse_post(&vm, 10, -3, 1u));
    CHECK(haeun_board_read(&vm, HAEUN_MOUSE_BASE) == 1u);
    CHECK((int32_t)haeun_board_read(&vm, HAEUN_MOUSE_BASE + 4u) == 10);
    CHECK((int32_t)haeun_board_read(&vm, HAEUN_MOUSE_BASE + 8u) == -3);
    CHECK(haeun_board_read(&vm, HAEUN_MOUSE_BASE + 12u) == 1u);
    haeun_board_write(&vm, HAEUN_MOUSE_BASE + 20u, 0u);
    CHECK(haeun_board_read(&vm, HAEUN_MOUSE_BASE) == 0u);
    return 0;
}

/** Feed rejects when the ring cannot hold the whole batch. */
static int test_keyboard_overflow(void) {
    uint8_t ram[64];
    haeun_vm vm;
    uint8_t batch[HAEUN_KBD_CAPACITY + 1u];
    uint8_t one = 0x2Au;
    size_t i;
    memset(ram, 0, sizeof(ram));
    memset(batch, 0x10, sizeof(batch));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(!haeun_vm_keyboard_feed(&vm, batch, sizeof(batch)));
    CHECK(haeun_board_read(&vm, HAEUN_KBD_BASE + 4u) == 0u);
    for (i = 0u; i < HAEUN_KBD_CAPACITY; ++i)
        CHECK(haeun_vm_keyboard_feed(&vm, &one, 1u));
    CHECK(!haeun_vm_keyboard_feed(&vm, &one, 1u));
    CHECK(vm.kbd_rx_count == HAEUN_KBD_CAPACITY);
    return 0;
}

/** CONTROL enables IRQC latch; mouse overwrite keeps latest sample. */
static int test_input_irq_and_mouse_overwrite(void) {
    uint8_t ram[64];
    haeun_vm vm;
    uint8_t key = 0x1Cu;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    haeun_board_write(&vm, HAEUN_KBD_BASE + 8u, 1u);
    CHECK(haeun_vm_keyboard_feed(&vm, &key, 1u));
    CHECK((vm.irq_pending & (1u << 3)) != 0u);
    CHECK(haeun_vm_mouse_post(&vm, 1, 2, 1u));
    CHECK(haeun_vm_mouse_post(&vm, 9, 8, 2u));
    CHECK((int32_t)haeun_board_read(&vm, HAEUN_MOUSE_BASE + 4u) == 9);
    CHECK((int32_t)haeun_board_read(&vm, HAEUN_MOUSE_BASE + 8u) == 8);
    CHECK(haeun_board_read(&vm, HAEUN_MOUSE_BASE + 12u) == 2u);
    haeun_board_write(&vm, HAEUN_MOUSE_BASE + 16u, 1u);
    CHECK((vm.irq_pending & (1u << 4)) != 0u);
    return 0;
}

int main(void) {
    if (test_boot_rom_protect() != 0) return 1;
    if (test_framebuffer_config() != 0) return 1;
    if (test_keyboard_mouse() != 0) return 1;
    if (test_keyboard_overflow() != 0) return 1;
    if (test_input_irq_and_mouse_overwrite() != 0) return 1;
    puts("all board extra device tests passed");
    return 0;
}
