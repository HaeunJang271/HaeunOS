#include "haeun/cpu_state.h"
#include "haeun/instruction_decoder.h"
#include "haeun/instruction_trace.h"
#include "haeun/register_file.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

/**
 * Verifies r0 hardwire and bounds behavior of the register file.
 */
static int test_register_file(void) {
    uint8_t ram[16];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_rf_write(&vm, 1u, 0x11u);
    haeun_rf_write(&vm, 0u, 0x99u);
    CHECK(haeun_rf_read(&vm, 1u) == 0x11u);
    CHECK(haeun_rf_read(&vm, 0u) == 0u);
    haeun_rf_clear_r0(&vm);
    CHECK(haeun_rf_read(&vm, 0u) == 0u);
    CHECK(haeun_rf_read(&vm, 99u) == 0u);
    return 0;
}

/**
 * Verifies decoder accepts ADD and rejects reserved bits / wrong ISA.
 */
static int test_decoder(void) {
    haeun_decoded_insn d;
    uint32_t cause = 1u;
    uint32_t add = (2u << 26) | (3u << 22) | (1u << 18) | (2u << 14);
    CHECK(haeun_decode_instruction(add, 1u, &d, &cause));
    CHECK(d.op == HAEUN_OP_ADD && d.rd == 3u && d.ra == 1u && d.rb == 2u);
    CHECK(!haeun_decode_instruction(add | 1u, 1u, &d, &cause));
    CHECK(cause == 0u);
    CHECK(!haeun_decode_instruction((0x12u << 26), 3u, &d, &cause));
    return 0;
}

/**
 * Verifies the optional instruction trace ring buffer.
 */
static int test_trace_ring(void) {
    haeun_instruction_trace trace;
    haeun_trace_entry e;
    uint32_t i;
    haeun_trace_clear(&trace);
    for (i = 0u; i < HAEUN_TRACE_CAPACITY + 3u; ++i)
        haeun_trace_record(&trace, i, i * 2u, (uint8_t)i, i + 4u);
    CHECK(haeun_trace_size(&trace) == HAEUN_TRACE_CAPACITY);
    CHECK(haeun_trace_at(&trace, 0u, &e));
    CHECK(e.pc == 3u);
    CHECK(haeun_trace_at(&trace, HAEUN_TRACE_CAPACITY - 1u, &e));
    CHECK(e.pc == HAEUN_TRACE_CAPACITY + 2u);
    CHECK(!haeun_trace_at(&trace, HAEUN_TRACE_CAPACITY, &e));
    return 0;
}

/**
 * Verifies cpu_state helpers after reset.
 */
static int test_cpu_state(void) {
    uint8_t ram[16];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 8u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_cpu_pc(&vm) == 8u);
    CHECK(!haeun_cpu_halted(&vm));
    haeun_cpu_set_halted(&vm, true);
    CHECK(haeun_cpu_halted(&vm));
    return 0;
}

int main(void) {
    if (test_register_file() != 0) return 1;
    if (test_decoder() != 0) return 1;
    if (test_trace_ring() != 0) return 1;
    if (test_cpu_state() != 0) return 1;
    puts("all CPU module tests passed");
    return 0;
}
