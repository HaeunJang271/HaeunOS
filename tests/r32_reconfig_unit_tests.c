/**
 * R3.2 host unit tests: transaction SM, prepare/commit/rollback, FI.
 */
#include "haeun/pval_types.h"
#include "haeun/reconfig.h"
#include "haeun/reconfig_eval.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE (1024u * 1024u)
#define PCB 0xA000u
#define STRIDE 0x100u
#define CURR 0xA400u
#define POL 0xA474u
#define IDLE 0xA460u

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
    vm->timer_count = 1000u;
    vm->timer_compare = 2000u;
    vm->pc = 0x100000u;
}

/** Idle RR fixture: no READY/RUNNING → vacuous FIFO timestamp. */
static void plant_idle_rr(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 1u);
    wr32(ram, PCB + 0u, 0u); /* FREE */
    wr32(ram, PCB + STRIDE, 3u); /* SLEEP */
    wr32(ram, PCB + 2u * STRIDE, 0u);
}

/** FCFS with one RUNNING — FIFO→RR does not need timestamps. */
static void plant_fcfs_running(uint8_t *ram) {
    wr32(ram, POL, 1u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 160u, 10u);
}

/** RR with RUNNING — RR→FIFO must reject (timestamp UNKNOWN). */
static void plant_rr_running(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 160u, 10u);
}

static int test_sm_and_ids(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn a, b;
    printf("RUN sm_ids\n");
    init_vm(&vm, ram);
    plant_idle_rr(ram);
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&a, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_request(&b, &vm, 0u, 1u) == HAEUN_RECFG_BUSY);
    CHECK(haeun_recfg_prepare(&a, &vm) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_commit(&a, &vm) == HAEUN_RECFG_OK);
    CHECK(a.state == HAEUN_RECFG_ST_COMMITTED);
    CHECK(rd32(ram, POL) == 1u);
    CHECK(haeun_recfg_prepare(&a, &vm) == HAEUN_RECFG_BAD_STATE);
    CHECK(haeun_recfg_request(&b, &vm, 1u, 0u) == HAEUN_RECFG_OK);
    CHECK(strcmp(a.txn_id, b.txn_id) != 0);
    CHECK(haeun_recfg_prepare(&b, &vm) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_commit(&b, &vm) == HAEUN_RECFG_OK);
    CHECK(rd32(ram, POL) == 0u);
    printf("sm_ids ok\n");
    return 0;
}

static int test_reject_paths(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn txn;
    uint32_t pol;
    printf("RUN reject_paths\n");
    init_vm(&vm, ram);
    plant_rr_running(ram);
    haeun_recfg_fi_clear();
    pol = rd32(ram, POL);
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_REJECTED);
    CHECK(rd32(ram, POL) == pol);
    CHECK(haeun_recfg_request(&txn, &vm, 9u, 1u) == HAEUN_RECFG_REJECTED);
    plant_idle_rr(ram);
    CHECK(haeun_recfg_request(&txn, &vm, 1u, 0u) == HAEUN_RECFG_REJECTED);
    printf("reject_paths ok\n");
    return 0;
}

static int test_stale_and_fi(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn txn;
    printf("RUN stale_fi\n");
    init_vm(&vm, ram);
    plant_idle_rr(ram);
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK);
    wr32(ram, PCB + 0u, 1u); /* mutate after prepare */
    CHECK(haeun_recfg_commit(&txn, &vm) == HAEUN_RECFG_REJECTED);
    plant_idle_rr(ram);
    haeun_recfg_fi_arm(HAEUN_RECFG_FI_AFTER_WRITE);
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_commit(&txn, &vm) == HAEUN_RECFG_FAILED);
    CHECK(rd32(ram, POL) == 1u);
    CHECK(haeun_recfg_rollback(&txn, &vm) == HAEUN_RECFG_OK);
    CHECK(txn.policy_id_restored && rd32(ram, POL) == 0u);
    CHECK(haeun_recfg_rollback(&txn, &vm) == HAEUN_RECFG_BAD_STATE);
    printf("stale_fi ok\n");
    return 0;
}

static int test_all_fi_points(void) {
    static const haeun_recfg_fi_point pts[] = {
        HAEUN_RECFG_FI_BEFORE_SNAPSHOT, HAEUN_RECFG_FI_AFTER_SNAPSHOT,
        HAEUN_RECFG_FI_AFTER_VALIDATE,  HAEUN_RECFG_FI_BEFORE_PREPARE,
        HAEUN_RECFG_FI_AFTER_PREPARE,   HAEUN_RECFG_FI_BEFORE_COMMIT,
        HAEUN_RECFG_FI_AFTER_WRITE,     HAEUN_RECFG_FI_AFTER_VERIFY,
        HAEUN_RECFG_FI_DURING_ROLLBACK, HAEUN_RECFG_FI_AFTER_ROLLBACK};
    unsigned i;
    printf("RUN all_fi\n");
    for (i = 0; i < sizeof(pts) / sizeof(pts[0]); ++i) {
        uint8_t ram[RAM_SIZE];
        haeun_vm vm;
        haeun_recfg_txn txn;
        haeun_recfg_result cr;
        init_vm(&vm, ram);
        plant_idle_rr(ram);
        haeun_recfg_fi_clear();
        if (pts[i] == HAEUN_RECFG_FI_DURING_ROLLBACK ||
            pts[i] == HAEUN_RECFG_FI_AFTER_ROLLBACK) {
            haeun_recfg_fi_arm(HAEUN_RECFG_FI_AFTER_WRITE);
            CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
            CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK);
            CHECK(haeun_recfg_commit(&txn, &vm) == HAEUN_RECFG_FAILED);
            haeun_recfg_fi_arm(pts[i]);
            (void)haeun_recfg_rollback(&txn, &vm);
            CHECK(txn.state == HAEUN_RECFG_ST_FAILED ||
                  txn.state == HAEUN_RECFG_ST_ROLLED_BACK);
            continue;
        }
        haeun_recfg_fi_arm(pts[i]);
        if (haeun_recfg_request(&txn, &vm, 0u, 1u) != HAEUN_RECFG_OK) {
            CHECK(txn.state == HAEUN_RECFG_ST_REJECTED);
            continue;
        }
        if (haeun_recfg_prepare(&txn, &vm) != HAEUN_RECFG_OK) {
            CHECK(txn.state == HAEUN_RECFG_ST_REJECTED);
            CHECK(rd32(ram, POL) == 0u);
            continue;
        }
        cr = haeun_recfg_commit(&txn, &vm);
        if (cr == HAEUN_RECFG_FAILED) {
            (void)haeun_recfg_rollback(&txn, &vm);
            CHECK(txn.state == HAEUN_RECFG_ST_ROLLED_BACK ||
                  txn.state == HAEUN_RECFG_ST_FAILED);
        } else {
            CHECK(cr == HAEUN_RECFG_REJECTED || cr == HAEUN_RECFG_OK);
        }
    }
    printf("all_fi ok\n");
    return 0;
}

static int test_eval_counters(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_recfg_txn txn;
    haeun_recfg_eval ev;
    printf("RUN eval_counters\n");
    haeun_recfg_eval_clear(&ev);
    init_vm(&vm, ram);
    plant_fcfs_running(ram);
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&txn, &vm, 1u, 0u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_commit(&txn, &vm) == HAEUN_RECFG_OK);
    haeun_recfg_eval_record(&ev, &txn, 10u, 5u, 0u, 0);
    CHECK(ev.success_count == 1u);
    plant_rr_running(ram);
    CHECK(haeun_recfg_request(&txn, &vm, 0u, 1u) == HAEUN_RECFG_OK);
    CHECK(haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_REJECTED);
    haeun_recfg_eval_record(&ev, &txn, 1u, 0u, 0u, 0);
    CHECK(ev.rejected_count == 1u);
    haeun_recfg_eval_fprint(stdout, &ev);
    printf("eval_counters ok\n");
    return 0;
}

/**
 * Entry for R3.2 reconfiguration unit tests.
 */
int main(void) {
    if (test_sm_and_ids() || test_reject_paths() || test_stale_and_fi() ||
        test_all_fi_points() || test_eval_counters())
        return 1;
    printf("r32 reconfig unit tests passed\n");
    return 0;
}
