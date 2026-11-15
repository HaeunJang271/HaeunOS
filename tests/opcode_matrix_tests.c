#include "haeun/instruction_decoder.h"
#include "haeun/instruction_trace.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))
#define CSR_IMM(funct,csr) (((uint32_t)(funct)<<12)|((uint32_t)(csr)&0xfffu))

static void put_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static int init_at(haeun_vm *vm, uint8_t *ram, size_t n, uint32_t isa) {
    memset(ram, 0, n);
    return haeun_vm_init_version(vm, ram, n, 0u, isa) == HAEUN_VM_OK ? 0 : 1;
}

/**
 * Every defined opcode retires once under the minimum ISA that allows it.
 */
static int test_every_opcode_smoke(void) {
    uint8_t ram[256];
    haeun_vm vm;
    haeun_instruction_trace trace;
    haeun_trace_entry entry;
    haeun_trace_clear(&trace);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    vm.trace = &trace;
    put_le32(ram, ENC(HAEUN_OP_NOP, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 4u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_HALT, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_HALTED);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ADD, 3, 1, 2, 0));
    vm.r[1] = 1u; vm.r[2] = 2u; vm.r[4] = 0xAAu;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 3u && vm.r[4] == 0xAAu);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_SUB, 3, 1, 2, 0));
    vm.r[1] = 5u; vm.r[2] = 3u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 2u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_AND, 3, 1, 2, 0));
    vm.r[1] = 0xFu; vm.r[2] = 0x3u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 0x3u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_OR, 3, 1, 2, 0));
    vm.r[1] = 0x4u; vm.r[2] = 0x1u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 0x5u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_XOR, 3, 1, 2, 0));
    vm.r[1] = 0x6u; vm.r[2] = 0x3u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 0x5u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ADDI, 3, 1, 0, 7));
    vm.r[1] = 10u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 17u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_LW, 3, 1, 0, 0));
    put_le32(ram + 32u, 0x11223344u);
    vm.r[1] = 32u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 0x11223344u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_SW, 3, 1, 0, 0));
    vm.r[1] = 32u; vm.r[3] = 0xAABBCCDDu;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(ram[32] == 0xDDu && ram[35] == 0xAAu);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_BEQ, 1, 2, 0, 1));
    vm.r[1] = 1u; vm.r[2] = 1u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 8u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_BEQ, 1, 2, 0, 1));
    vm.r[1] = 1u; vm.r[2] = 2u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 4u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_BNE, 1, 2, 0, 1));
    vm.r[1] = 1u; vm.r[2] = 2u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 8u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_JAL, 5, 0, 0, 1));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[5] == 4u && vm.pc == 8u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ECALL, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_TRAP && vm.cause == 3u);

    CHECK(init_at(&vm, ram, sizeof(ram), 2u) == 0);
    put_le32(ram, ENC(HAEUN_OP_JALR, 15, 1, 0, 0));
    put_le32(ram + 8u, ENC(HAEUN_OP_HALT, 0, 0, 0, 0));
    vm.r[1] = 8u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 8u && vm.r[15] == 4u);

    CHECK(init_at(&vm, ram, sizeof(ram), 3u) == 0);
    put_le32(ram, ENC(HAEUN_OP_CMP, 3, 1, 2, 0u << 11));
    vm.r[1] = 9u; vm.r[2] = 9u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.r[3] == 1u);

    CHECK(init_at(&vm, ram, sizeof(ram), 4u) == 0);
    put_le32(ram, ENC(HAEUN_OP_CSR, 2, 1, 0, CSR_IMM(1, 5))); /* CSRRW scratch */
    vm.r[1] = 0x55u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.csr_scratch == 0x55u);

    CHECK(init_at(&vm, ram, sizeof(ram), 4u) == 0);
    put_le32(ram, ENC(HAEUN_OP_SFENCE, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 4u);

    CHECK(init_at(&vm, ram, sizeof(ram), 4u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ERET, 0, 0, 0, 0));
    vm.csr_epc = 16u;
    vm.csr_status = 0u; /* PP=user */
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK && vm.pc == 16u);
    CHECK(vm.privilege == HAEUN_PRIV_USER);

    CHECK(haeun_trace_size(&trace) >= 1u);
    CHECK(haeun_trace_at(&trace, 0u, &entry));
    CHECK(entry.op == HAEUN_OP_NOP);
    return 0;
}

/**
 * Invalid encodings and privilege/version gates for each opcode class.
 */
static int test_invalid_and_privilege(void) {
    uint8_t ram[128];
    haeun_vm vm;

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ADD, 1, 1, 2, 1)); /* reserved low bits */
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ILLEGAL_INSTRUCTION);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_HALT, 0, 0, 0, 1));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ILLEGAL_INSTRUCTION);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_JALR, 1, 1, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ILLEGAL_INSTRUCTION);

    CHECK(init_at(&vm, ram, sizeof(ram), 2u) == 0);
    put_le32(ram, ENC(HAEUN_OP_CMP, 1, 1, 2, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ILLEGAL_INSTRUCTION);

    CHECK(init_at(&vm, ram, sizeof(ram), 3u) == 0);
    put_le32(ram, ENC(HAEUN_OP_SFENCE, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ILLEGAL_INSTRUCTION);

    CHECK(init_at(&vm, ram, sizeof(ram), 4u) == 0);
    put_le32(ram, ENC(HAEUN_OP_ERET, 0, 0, 0, 0));
    vm.privilege = HAEUN_PRIV_USER;
    vm.csr_trapvec = 32u;
    put_le32(ram + 32u, ENC(HAEUN_OP_HALT, 0, 0, 0, 0));
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 32u && vm.csr_cause == 4u);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_LW, 1, 2, 0, 0));
    vm.r[2] = 1u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_ALIGNMENT_FAULT);

    CHECK(init_at(&vm, ram, sizeof(ram), 1u) == 0);
    put_le32(ram, ENC(HAEUN_OP_SW, 1, 2, 0, 0));
    vm.r[2] = 512u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_MEMORY_FAULT);
    return 0;
}

/**
 * Decoder rejects every unused major opcode above SFENCE for ISA 4.
 */
static int test_unused_opcodes_illegal(void) {
    haeun_decoded_insn d;
    uint32_t cause = 0u;
    uint32_t op;
    for (op = HAEUN_OP_SFENCE + 1u; op < 64u; ++op) {
        CHECK(!haeun_decode_instruction(op << 26, 4u, &d, &cause));
        CHECK(cause == 0u);
    }
    return 0;
}

int main(void) {
    if (test_every_opcode_smoke() != 0) return 1;
    if (test_invalid_and_privilege() != 0) return 1;
    if (test_unused_opcodes_illegal() != 0) return 1;
    puts("all opcode matrix tests passed");
    return 0;
}
