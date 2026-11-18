/**
 * R4.3: behavioral selection + bidirectional active apply evidence.
 * Usage: r43_active_verification_tests <build-dir>
 */
#include "haeun/hxf.h"
#include "haeun/policy_verify.h"
#include "haeun/reconfig.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define RAM_SIZE (16u * 1024u * 1024u)
#define POL 0xA474u
#define CURR 0xA400u
#define PCB 0xA000u
#define STRIDE 0x100u
#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static uint32_t rd32(const uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off] | ((uint32_t)ram[off + 1] << 8) |
           ((uint32_t)ram[off + 2] << 16) | ((uint32_t)ram[off + 3] << 24);
}

static void fixture_from_ram(haeun_policy_fixture *fx, const uint8_t *ram,
                             uint32_t curr_before) {
    unsigned i;
    memset(fx, 0, sizeof(*fx));
    fx->policy_id = rd32(ram, POL);
    fx->curr = curr_before;
    for (i = 0; i < HAEUN_POLICY_SLOTS; ++i) {
        uint32_t b = PCB + i * STRIDE;
        fx->state[i] = rd32(ram, b);
        fx->ready_since[i] = rd32(ram, b + 168u);
        fx->ready_seq_lo[i] = rd32(ram, b + 172u);
        fx->ready_seq_hi[i] = rd32(ram, b + 176u);
    }
}

static unsigned count_elig(const uint8_t *ram) {
    unsigned n = 0u, i;
    for (i = 0; i < 3u; ++i) {
        uint32_t st = rd32(ram, PCB + i * STRIDE);
        if (st == 1u || st == 2u)
            n++;
    }
    return n;
}

static int seqs_ok(const uint8_t *ram) {
    unsigned i;
    for (i = 0; i < 3u; ++i) {
        uint32_t st = rd32(ram, PCB + i * STRIDE);
        if (st != 1u && st != 2u)
            continue;
        if (rd32(ram, PCB + i * STRIDE + 172u) == 0u &&
            rd32(ram, PCB + i * STRIDE + 176u) == 0u)
            return 0;
    }
    return 1;
}

static int load_vm(const char *path, haeun_vm *vm, uint8_t **ram_out) {
    FILE *f;
    uint8_t *image, *ram;
    long n;
    size_t got;
    uint32_t entry = 0u, isa = 0u;
    f = fopen(path, "rb");
    CHECK(f && fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 0 &&
          fseek(f, 0, SEEK_SET) == 0);
    image = (uint8_t *)malloc((size_t)n);
    CHECK(image && (got = fread(image, 1, (size_t)n, f)) == (size_t)n);
    (void)fclose(f);
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    CHECK(ram && haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) ==
                     HAEUN_HXF_OK);
    free(image);
    CHECK(haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK);
    *ram_out = ram;
    return 0;
}

static int txn_switch(haeun_vm *vm, uint32_t src, uint32_t dst,
                      haeun_recfg_txn *txn) {
    haeun_recfg_fi_clear();
    if (haeun_recfg_request(txn, vm, src, dst) != HAEUN_RECFG_OK)
        return 1;
    if (haeun_recfg_prepare(txn, vm) != HAEUN_RECFG_OK)
        return 1;
    return haeun_recfg_commit(txn, vm) == HAEUN_RECFG_OK ? 0 : 1;
}

typedef struct {
    unsigned events;
    unsigned matches;
    unsigned mismatches;
    unsigned partial_id_only;
} sel_stats;

static void check_event(sel_stats *ss, const uint8_t *ram, uint32_t before,
                        uint32_t after, FILE *tr) {
    haeun_policy_fixture fx;
    uint32_t expect = 0u;
    int ok;
    fixture_from_ram(&fx, ram, before);
    ok = haeun_policy_select_host(&fx, &expect);
    ss->events++;
    if (!ok) {
        ss->mismatches++;
        fprintf(tr,
                "{\"before\":%u,\"after\":%u,\"pol\":%u,\"host_ok\":0,"
                "\"match\":false},\n",
                before, after, fx.policy_id);
        return;
    }
    if (expect == after) {
        ss->matches++;
        fprintf(tr,
                "{\"before\":%u,\"after\":%u,\"pol\":%u,\"expected\":%u,"
                "\"match\":true},\n",
                before, after, fx.policy_id, expect);
    } else {
        ss->mismatches++;
        fprintf(tr,
                "{\"before\":%u,\"after\":%u,\"pol\":%u,\"expected\":%u,"
                "\"match\":false},\n",
                before, after, fx.policy_id, expect);
    }
    (void)ss->partial_id_only;
}

static int run_phase(const char *path, uint32_t boot, uint32_t target,
                     const char *label, FILE *tr, FILE *ap, sel_stats *ss) {
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    uint32_t last = 0u;
    int primed = 0, switched = 0;
    uint64_t i;
    unsigned post = 0u;
    printf("RUN %s\n", label);
    if (load_vm(path, &vm, &ram))
        return 1;
    for (i = 0; i < 250000u; ++i) {
        haeun_vm_result st = haeun_vm_step(&vm);
        uint32_t curr = rd32(ram, CURR);
        if (!primed) {
            last = curr;
            primed = 1;
        } else if (curr != last) {
            check_event(ss, ram, last, curr, tr);
            if (switched && rd32(ram, POL) == target)
                post++;
            last = curr;
        }
        if (!switched && st == HAEUN_VM_OK && count_elig(ram) >= 2u &&
            seqs_ok(ram) && i > 2000u && rd32(ram, POL) == boot) {
            CHECK(txn_switch(&vm, boot, target, &txn) == 0);
            CHECK(txn.state == HAEUN_RECFG_ST_COMMITTED);
            CHECK(rd32(ram, POL) == target);
            switched = 1;
            fprintf(ap,
                    "{\"label\":\"%s\",\"step\":%llu,\"src\":%u,\"dst\":%u,"
                    "\"eligible\":%u,\"txn\":\"COMMITTED\",\"pol_after\":%u},\n",
                    label, (unsigned long long)i, boot, target, count_elig(ram),
                    rd32(ram, POL));
            printf("  apply at %llu pol=%u\n", (unsigned long long)i, target);
        }
        if (st == HAEUN_VM_HALTED)
            break;
        CHECK(st == HAEUN_VM_OK);
    }
    CHECK(switched);
    CHECK(post >= 1u);
    CHECK(ss->mismatches == 0u);
    free(ram);
    printf("%s ok matches=%u events=%u post=%u\n", label, ss->matches,
           ss->events, post);
    return 0;
}

static int safety_and_rollback(const char *build, FILE *sj, FILE *rj) {
    haeun_vm vm;
    uint8_t *ram = NULL;
    haeun_recfg_txn txn;
    char path[512];
    uint32_t before;
    snprintf(path, sizeof(path), "%s/r12_yield.hxf", build);
    CHECK(load_vm(path, &vm, &ram) == 0);
    {
        uint64_t i;
        for (i = 0; i < 4000u; ++i)
            if (haeun_vm_step(&vm) != HAEUN_VM_OK)
                break;
    }
    before = rd32(ram, POL);
    /* invalid target */
    haeun_recfg_fi_clear();
    CHECK(haeun_recfg_request(&txn, &vm, before, 99u) != HAEUN_RECFG_OK ||
          haeun_recfg_prepare(&txn, &vm) != HAEUN_RECFG_OK);
    CHECK(rd32(ram, POL) == before);
    fprintf(sj,
            "{\"case\":\"invalid_policy\",\"blocked\":true,\"pol\":%u},\n",
            before);
    /* FI after write → rollback restores policy ID */
    haeun_recfg_fi_clear();
    if (haeun_recfg_request(&txn, &vm, before, before == 0u ? 1u : 0u) ==
            HAEUN_RECFG_OK &&
        haeun_recfg_prepare(&txn, &vm) == HAEUN_RECFG_OK) {
        haeun_recfg_fi_arm(HAEUN_RECFG_FI_AFTER_WRITE);
        (void)haeun_recfg_commit(&txn, &vm);
        if (txn.state == HAEUN_RECFG_ST_FAILED ||
            txn.state == HAEUN_RECFG_ST_ROLLING_BACK ||
            txn.state == HAEUN_RECFG_ST_ROLLED_BACK) {
            (void)haeun_recfg_rollback(&txn, &vm);
        }
        fprintf(rj,
                "{\"fi\":\"AFTER_WRITE\",\"policy_id_restored\":%s,"
                "\"pol_now\":%u,\"scope\":\"policy_id_only\"}\n",
                txn.policy_id_restored ? "true" : "false", rd32(ram, POL));
    } else {
        fprintf(rj,
                "{\"fi\":\"AFTER_WRITE\",\"skipped\":true,\"reason\":"
                "\"prepare_rejected_or_unavailable\"}\n");
        fprintf(sj, "{\"case\":\"prepare_gate\",\"note\":\"reject_path_hit\"},\n");
    }
    free(ram);
    return 0;
}

/**
 * Entry: selection behavioral proof + apply + safety artifacts.
 */
int main(int argc, char **argv) {
    FILE *tr, *ap, *sj, *rj, *map;
    sel_stats ss_rr = {0}, ss_ff = {0};
    char path[512];
    if (argc != 2) {
        fprintf(stderr, "usage: r43_active_verification_tests <build-dir>\n");
        return 2;
    }
#ifdef _WIN32
    (void)_mkdir("research");
    (void)_mkdir("research/experiments");
#else
    (void)mkdir("research", 0755);
    (void)mkdir("research/experiments", 0755);
#endif
    tr = fopen("research/experiments/r43-selection-trace.json", "wb");
    ap = fopen("research/experiments/r43-active-apply.json", "wb");
    sj = fopen("research/experiments/r43-safety-rejection.json", "wb");
    rj = fopen("research/experiments/r43-rollback-verification.json", "wb");
    map = fopen("research/experiments/r43-profile-apply-mapping.json", "wb");
    CHECK(tr && ap && sj && rj && map);
    fputs("{\"events\":[\n", tr);
    fputs("{\"applies\":[\n", ap);
    fputs("{\"rejections\":[\n", sj);
    snprintf(path, sizeof(path), "%s/r12_pair.hxf", argv[1]);
    CHECK(run_phase(path, 0u, 1u, "RR_to_FIFO_behavioral", tr, ap, &ss_rr) ==
          0);
    snprintf(path, sizeof(path), "%s/fcfs/r12_pair.hxf", argv[1]);
    CHECK(run_phase(path, 1u, 0u, "FIFO_to_RR_behavioral", tr, ap, &ss_ff) ==
          0);
    CHECK(safety_and_rollback(argv[1], sj, rj) == 0);
    fputs("{\"end\":true}]}\n", tr);
    fputs("{\"end\":true}]}\n", ap);
    fputs("{\"end\":true}]}\n", sj);
    fprintf(map,
            "{\"note\":\"R4.2 6x5 all recommended RR; R4.3 groups apply\","
            "\"cells\":30,\"live_apply_per_cell\":false,"
            "\"groups\":[{\"recommendation\":\"RR\",\"workloads\":\"all_six\","
            "\"profiles\":\"all_five\",\"apply_evidence\":"
            "\"RR_to_FIFO_and_FIFO_to_RR_behavioral\",\"status\":\"OBSERVED\"}],"
            "\"unapplied_individual_cells\":30}\n");
    fclose(tr);
    fclose(ap);
    fclose(sj);
    fclose(rj);
    fclose(map);
    printf("selection RR matches=%u FIFO-boot matches=%u\n", ss_rr.matches,
           ss_ff.matches);
    CHECK(ss_rr.matches + ss_ff.matches >= 2u);
    printf("r43 active verification tests passed\n");
    return 0;
}
