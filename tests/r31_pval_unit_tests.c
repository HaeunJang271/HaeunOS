/**
 * R3.1 unit tests: planted fixtures for invariants, timestamps, compat, reports.
 */
#include "haeun/pval_compat.h"
#include "haeun/pval_invariants.h"
#include "haeun/pval_report.h"
#include "haeun/pval_snapshot.h"
#include "haeun/pval_timestamp.h"
#include "haeun/pval_validate.h"
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

static void init_vm(haeun_vm *vm, uint8_t *ram) {
    memset(ram, 0, RAM_SIZE);
    memset(vm, 0, sizeof(*vm));
    vm->ram = ram;
    vm->ram_size = RAM_SIZE;
    vm->timer_count = 1000u;
    vm->timer_compare = 2000u;
    vm->pc = 0x100000u;
}

static int test_snapshot_and_readonly(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_pval_snapshot snap;
    printf("RUN snapshot_readonly\n");
    init_vm(&vm, ram);
    wr32(ram, POL, 0u);
    wr32(ram, CURR, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + 28u, 0x2001u);
    CHECK(haeun_pval_snapshot_read(&vm, &snap) == HAEUN_PVAL_OK);
    CHECK(snap.policy_id == 0u && snap.slots[0].state == 2u);
    CHECK(!snap.slots[3].is_participant);
    CHECK(haeun_pval_snapshot_verify_unchanged(&vm, &snap) == HAEUN_PVAL_OK);
    wr32(ram, POL, 1u);
    CHECK(haeun_pval_snapshot_verify_unchanged(&vm, &snap) == HAEUN_PVAL_MUTATION);
    printf("snapshot_readonly ok\n");
    return 0;
}

static int test_timestamps(void) {
    haeun_pval_snapshot snap;
    haeun_pval_timestamp_report ts;
    printf("RUN timestamps\n");
    memset(&snap, 0, sizeof(snap));
    snap.slots[0].as_slot_valid = 1;
    snap.slots[0].state = 1;
    snap.slots[0].ready_since = 10u;
    snap.slots[1].as_slot_valid = 1;
    snap.slots[1].state = 1;
    snap.slots[1].ready_since = 20u;
    snap.slots[2].as_slot_valid = 1;
    /* Legacy ready_since-only: still UNKNOWN. */
    CHECK(haeun_pval_timestamp_check(&snap, &ts) == HAEUN_PVAL_OK);
    CHECK(ts.overall == HAEUN_PVAL_UNKNOWN);
    /* R3.4 ready_seq unique + wrap==0 → PASS. */
    snap.slots[0].ready_seq_lo = 1u;
    snap.slots[1].ready_seq_lo = 2u;
    CHECK(haeun_pval_timestamp_check(&snap, &ts) == HAEUN_PVAL_OK);
    CHECK(ts.overall == HAEUN_PVAL_PASS && ts.external_bound_proven);
    snap.slots[1].ready_seq_lo = 1u;
    CHECK(haeun_pval_timestamp_check(&snap, &ts) == HAEUN_PVAL_OK);
    CHECK(ts.overall == HAEUN_PVAL_FAIL);
    snap.slots[1].ready_seq_lo = 2u;
    snap.ready_seq_wrap = 1u;
    CHECK(haeun_pval_timestamp_check(&snap, &ts) == HAEUN_PVAL_OK);
    CHECK(ts.overall == HAEUN_PVAL_UNKNOWN);
    snap.ready_seq_wrap = 0u;
    snap.slots[0].state = 3;
    snap.slots[1].state = 3;
    CHECK(haeun_pval_timestamp_check(&snap, &ts) == HAEUN_PVAL_OK);
    CHECK(ts.overall == HAEUN_PVAL_NOT_APPLICABLE);
    printf("timestamps ok\n");
    return 0;
}

static int test_compat_and_aggregate(void) {
    haeun_pval_snapshot snap;
    haeun_pval_timestamp_report ts;
    haeun_pval_compat_report cmp;
    haeun_pval_status parts[3];
    printf("RUN compat_aggregate\n");
    memset(&snap, 0, sizeof(snap));
    snap.policy_id = HAEUN_PVAL_POLICY_RR;
    memset(&ts, 0, sizeof(ts));
    ts.overall = HAEUN_PVAL_UNKNOWN;
    ts.pairwise_comparable = 1;
    CHECK(haeun_pval_compat_check(&snap, HAEUN_PVAL_POLICY_FCFS, &ts, &cmp) ==
          HAEUN_PVAL_OK);
    CHECK(cmp.overall == HAEUN_PVAL_UNKNOWN);
    CHECK(!cmp.transition_preconditions_satisfied);
    CHECK(haeun_pval_compat_check(&snap, 99u, &ts, &cmp) == HAEUN_PVAL_OK);
    CHECK(cmp.overall == HAEUN_PVAL_FAIL);
    parts[0] = HAEUN_PVAL_PASS;
    parts[1] = HAEUN_PVAL_UNKNOWN;
    parts[2] = HAEUN_PVAL_NOT_APPLICABLE;
    CHECK(haeun_pval_aggregate(parts, 3u) == HAEUN_PVAL_UNKNOWN);
    parts[1] = HAEUN_PVAL_FAIL;
    CHECK(haeun_pval_aggregate(parts, 3u) == HAEUN_PVAL_FAIL);
    printf("compat_aggregate ok\n");
    return 0;
}

static int test_validate_planted(void) {
    uint8_t ram[RAM_SIZE];
    haeun_vm vm;
    haeun_pval_report rep;
    char buf[4096];
    FILE *mem;
    size_t n;
    printf("RUN validate_planted\n");
    init_vm(&vm, ram);
    wr32(ram, POL, 1u);
    wr32(ram, CURR, 0u);
    wr32(ram, IDLE, 0u);
    wr32(ram, PCB + 0u, 2u);
    wr32(ram, PCB + 4u, 1u);
    wr32(ram, PCB + STRIDE, 1u);
    wr32(ram, PCB + STRIDE + 4u, 2u);
    wr32(ram, PCB + 160u, 100u);
    wr32(ram, PCB + STRIDE + 160u, 50u);
    CHECK(haeun_pval_validate(&vm, HAEUN_PVAL_POLICY_RR, &rep) == HAEUN_PVAL_OK);
    CHECK(rep.read_only == HAEUN_PVAL_PASS);
    CHECK(rep.snap.policy_id == 1u);
    CHECK(rep.overall != HAEUN_PVAL_PASS); /* external bound UNKNOWN */
    mem = tmpfile();
    CHECK(mem != NULL);
    haeun_pval_report_fprint(mem, &rep);
    haeun_pval_report_fprint_json(mem, &rep);
    rewind(mem);
    n = fread(buf, 1u, sizeof(buf) - 1u, mem);
    buf[n] = '\0';
    (void)fclose(mem);
    CHECK(strstr(buf, "\"schema_version\":1") != NULL);
    CHECK(strstr(buf, "UNKNOWN") != NULL);
    printf("validate_planted ok\n");
    return 0;
}

/**
 * Entry for R3.1 host validation unit tests.
 */
int main(void) {
    if (test_snapshot_and_readonly() || test_timestamps() ||
        test_compat_and_aggregate() || test_validate_planted())
        return 1;
    printf("r31 pval unit tests passed\n");
    return 0;
}
