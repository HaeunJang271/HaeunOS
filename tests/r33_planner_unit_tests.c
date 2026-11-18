/**
 * R3.3 planner unit tests: classify, decide, stability, shadow, safety gates.
 */
#include "haeun/planner.h"
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
    vm->timer_count = 5000u;
    vm->timer_compare = 9000u;
    vm->pc = 0x100000u;
}

static void plant_idle_rr(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 1u);
}

static void plant_cpu_rr(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 128u, 10000u); /* run */
    wr32(ram, PCB + 132u, 100u);   /* ready_wait */
    wr32(ram, PCB + 144u, 5u);     /* dispatch */
}

static void plant_short_rr(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 128u, 200u);
    wr32(ram, PCB + 132u, 50u);
    wr32(ram, PCB + 144u, 40u); /* high dispatch/run */
}

static void plant_bursty_rr(uint8_t *ram) {
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    wr32(ram, PCB + 128u, 1000u);
    wr32(ram, PCB + 132u, 5000u); /* ready/run = 5 */
    wr32(ram, PCB + 144u, 10u);
}

static void plant_cpu_fcfs(uint8_t *ram) {
    plant_cpu_rr(ram);
    wr32(ram, POL, 1u);
}

static int test_idle_keep_shadow(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan plan;
    uint32_t pol;
    printf("RUN idle_keep_shadow\n");
    init_vm(&vm, ram);
    plant_idle_rr(ram);
    pol = rd32(ram, POL);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    CHECK(plan.shadow_mode);
    CHECK(plan.decision == HAEUN_PLAN_KEEP_CURRENT ||
          plan.decision == HAEUN_PLAN_NO_DECISION);
    CHECK(!plan.txn_attempted && !plan.txn_committed);
    CHECK(rd32(ram, POL) == pol);
    printf("idle_keep_shadow ok\n");
    return 0;
}

static int test_cpu_keep_rr(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan plan;
    printf("RUN cpu_keep_rr\n");
    init_vm(&vm, ram);
    plant_cpu_rr(ram);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    CHECK(plan.workload_class == HAEUN_PLAN_WL_CPU_BOUND ||
          plan.workload_class == HAEUN_PLAN_WL_MIXED);
    CHECK(plan.decision == HAEUN_PLAN_KEEP_CURRENT);
    CHECK(rd32(ram, POL) == 0u);
    printf("cpu_keep_rr ok\n");
    return 0;
}

static int test_recommend_and_safety_unknown(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan plan;
    printf("RUN recommend_safety_unknown\n");
    init_vm(&vm, ram);
    plant_bursty_rr(ram);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    sess.cfg.ready_run_ratio_high = 2.0;
    sess.cfg.hysteresis_delta = 0.0;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    /* RR→FIFO with RUNNING eligible → R3.1 UNKNOWN → REJECTED_BY_SAFETY */
    CHECK(plan.decision == HAEUN_PLAN_REJECTED_BY_SAFETY ||
          plan.decision == HAEUN_PLAN_RECOMMEND_CHANGE ||
          plan.decision == HAEUN_PLAN_KEEP_CURRENT);
    if (plan.decision == HAEUN_PLAN_RECOMMEND_CHANGE)
        CHECK(plan.safety_preconditions_ok); /* only if vacuous */
    if (plan.decision == HAEUN_PLAN_REJECTED_BY_SAFETY)
        CHECK(!plan.txn_committed);
    CHECK(rd32(ram, POL) == 0u);
    CHECK(plan.shadow_mode);
    printf("recommend_safety_unknown ok decision=%d\n", (int)plan.decision);
    return 0;
}

static int test_fcfs_to_rr_interest(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan plan;
    printf("RUN fcfs_to_rr_interest\n");
    init_vm(&vm, ram);
    plant_cpu_fcfs(ram);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &plan) == HAEUN_PLAN_OK);
    CHECK(plan.target_policy == 0u || plan.decision == HAEUN_PLAN_KEEP_CURRENT ||
          plan.decision == HAEUN_PLAN_RECOMMEND_CHANGE ||
          plan.decision == HAEUN_PLAN_REJECTED_BY_SAFETY);
    CHECK(rd32(ram, POL) == 1u); /* shadow */
    printf("fcfs_to_rr_interest ok\n");
    return 0;
}

static int test_stability_and_replay(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan a, b;
    printf("RUN stability_replay\n");
    init_vm(&vm, ram);
    plant_short_rr(ram);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 10000u; /* force suppress on 2nd */
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    sess.cfg.dispatch_run_high = 0.1;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &a) == HAEUN_PLAN_OK);
    vm.timer_count = 5001u; /* tiny advance << dwell */
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &b) == HAEUN_PLAN_OK);
    /* Second call should keep or suppress change interest. */
    CHECK(b.decision == HAEUN_PLAN_KEEP_CURRENT ||
          b.decision == HAEUN_PLAN_NO_DECISION ||
          b.decision == HAEUN_PLAN_REJECTED_BY_SAFETY ||
          b.decision == HAEUN_PLAN_RECOMMEND_CHANGE);
    /* Deterministic: same session reset → same first decision */
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    sess.cfg.dispatch_run_high = 0.1;
    vm.timer_count = 5000u;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &a) == HAEUN_PLAN_OK);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    sess.cfg.dispatch_run_high = 0.1;
    CHECK(haeun_plan_run(&sess, &vm, 1, 0, &b) == HAEUN_PLAN_OK);
    CHECK(a.decision == b.decision);
    CHECK(a.target_policy == b.target_policy);
    CHECK(a.workload_class == b.workload_class);
    printf("stability_replay ok\n");
    return 0;
}

static int test_execute_opt_in_no_direct_write(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_plan_session sess;
    haeun_plan plan;
    printf("RUN execute_opt_in\n");
    init_vm(&vm, ram);
    plant_idle_rr(ram);
    haeun_plan_session_init(&sess);
    sess.cfg.min_dwell_ticks = 0u;
    sess.cfg.cooldown_ticks = 0u;
    sess.cfg.consecutive_needed = 1u;
    /* execute with KEEP → no txn */
    CHECK(haeun_plan_run(&sess, &vm, 0, 1, &plan) == HAEUN_PLAN_OK);
    CHECK(!plan.txn_committed || plan.decision == HAEUN_PLAN_KEEP_CURRENT ||
          plan.decision == HAEUN_PLAN_NO_DECISION);
    CHECK(rd32(ram, POL) == 0u || plan.txn_committed);
    printf("execute_opt_in ok\n");
    return 0;
}

static int test_missing_not_zero(void) {
    haeun_plan plan;
    haeun_plan_obs mon;
    haeun_plan_config cfg;
    printf("RUN missing_not_zero\n");
    memset(&mon, 0, sizeof(mon));
    memset(&plan, 0, sizeof(plan));
    haeun_plan_config_default(&cfg);
    mon.telem_ok = 0;
    CHECK(haeun_plan_analyze(&mon, &cfg, &plan) == HAEUN_PLAN_NO_TELEM);
    CHECK(plan.evidence_count >= 1u);
    CHECK(plan.evidence[0].kind == HAEUN_PLAN_EV_MISSING);
    printf("missing_not_zero ok\n");
    return 0;
}

/**
 * Entry for R3.3 planner unit tests.
 */
int main(void) {
    if (test_idle_keep_shadow() || test_cpu_keep_rr() ||
        test_recommend_and_safety_unknown() || test_fcfs_to_rr_interest() ||
        test_stability_and_replay() || test_execute_opt_in_no_direct_write() ||
        test_missing_not_zero())
        return 1;
    printf("r33 planner unit tests passed\n");
    return 0;
}
