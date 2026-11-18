/**
 * R3.4 unit: ready_seq FIFO mirror, active planted RR→FIFO commit.
 */
#include "haeun/policy_verify.h"
#include "haeun/pval_types.h"
#include "haeun/reconfig.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE (1024u * 1024u)
#define PCB 0xA000u
#define STRIDE 0x100u
#define CURR 0xA400u
#define POL 0xA474u
#define IDLE 0xA460u
#define SEQG 0xA46Cu
#define WRAP 0xA478u

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void wr32(uint8_t *ram, uint32_t off, uint32_t v) {
    ram[off] = (uint8_t)(v & 0xffu);
    ram[off + 1u] = (uint8_t)((v >> 8) & 0xffu);
    ram[off + 2u] = (uint8_t)((v >> 16) & 0xffu);
    ram[off + 3u] = (uint8_t)((v >> 24) & 0xffu);
}

static uint32_t rd32(const uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off] | ((uint32_t)ram[off + 1u] << 8) |
           ((uint32_t)ram[off + 2u] << 16) | ((uint32_t)ram[off + 3u] << 24);
}

static void init_vm(haeun_vm *vm, uint8_t *ram) {
    memset(ram, 0, RAM_SIZE);
    memset(vm, 0, sizeof(*vm));
    vm->ram = ram;
    vm->ram_size = RAM_SIZE;
    vm->timer_count = 5000u;
    vm->timer_compare = 9000u;
}

static int test_seq_fifo_mirror(void) {
    haeun_policy_fixture fx;
    uint32_t slot = 99u;
    printf("RUN seq_fifo_mirror\n");
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_FCFS;
    fx.state[0] = 1u;
    fx.state[1] = 1u;
    fx.state[2] = 1u;
    fx.ready_seq_lo[0] = 3u;
    fx.ready_seq_lo[1] = 1u; /* oldest */
    fx.ready_seq_lo[2] = 2u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 1);
    CHECK(slot == 1u);
    fx.ready_seq_lo[0] = 5u;
    fx.ready_seq_lo[1] = 5u;
    fx.ready_seq_lo[2] = 9u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 1);
    CHECK(slot == 0u); /* tie → lower slot */
    printf("seq_fifo_mirror ok\n");
    return 0;
}

/** Two READY + one RUNNING under RR with distinct seq → active RR→FIFO. */
static int test_active_rr_to_fifo_planted(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn txn;
    printf("RUN active_rr_to_fifo_planted\n");
    init_vm(&vm, ram);
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, WRAP, 0u);
    wr32(ram, SEQG, 3u);
    /* slot0 RUNNING seq=1, slot1 READY seq=2, slot2 READY seq=3 */
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 172u, 1u);
    wr32(ram, PCB + STRIDE, 1u);
    wr32(ram, PCB + STRIDE + 4u, 2u);
    wr32(ram, PCB + STRIDE + 28u, 0x6001u);
    wr32(ram, PCB + STRIDE + 172u, 2u);
    wr32(ram, PCB + 2u * STRIDE, 1u);
    wr32(ram, PCB + 2u * STRIDE + 4u, 3u);
    wr32(ram, PCB + 2u * STRIDE + 28u, 0x8001u);
    wr32(ram, PCB + 2u * STRIDE + 172u, 3u);
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_commit(&txn, &vm) == HAEUN_RECFG_OK);
    CHECK(rd32(ram, POL) == 1u);
    CHECK(txn.state == HAEUN_RECFG_ST_COMMITTED);
    printf("active_rr_to_fifo_planted ok\n");
    return 0;
}

static int test_wrap_rejects(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn txn;
    printf("RUN wrap_rejects\n");
    init_vm(&vm, ram);
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, WRAP, 1u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 172u, 1u);
    wr32(ram, PCB + STRIDE, 1u);
    wr32(ram, PCB + STRIDE + 4u, 2u);
    wr32(ram, PCB + STRIDE + 28u, 0x6001u);
    wr32(ram, PCB + STRIDE + 172u, 2u);
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_REJECTED);
    CHECK(rd32(ram, POL) == 0u);
    printf("wrap_rejects ok\n");
    return 0;
}

/**
 * Entry for R3.4 active unit tests.
 */
int main(void) {
    if (test_seq_fifo_mirror() || test_active_rr_to_fifo_planted() ||
        test_wrap_rejects())
        return 1;
    printf("r34 active unit tests passed\n");
    return 0;
}
