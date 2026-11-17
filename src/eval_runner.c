#include "haeun/eval_runner.h"

#include "haeun/eval_metrics.h"
#include "haeun/eval_workload.h"
#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    RAM_SIZE = 16u * 1024u * 1024u,
    POL_OFF = 0x0000A474u
};

static uint32_t rd32(const uint8_t *ram, uint32_t off) {
    return (uint32_t)ram[off] | ((uint32_t)ram[off + 1u] << 8) |
           ((uint32_t)ram[off + 2u] << 16) | ((uint32_t)ram[off + 3u] << 24);
}

static int load_hxf(const char *path, haeun_vm *vm, uint8_t *ram) {
    FILE *f;
    uint8_t *image;
    long n;
    size_t got;
    uint32_t entry = 0u;
    uint32_t isa = 0u;
    f = fopen(path, "rb");
    if (f == NULL)
        return 1;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        (void)fclose(f);
        return 1;
    }
    image = (uint8_t *)malloc((size_t)n);
    if (image == NULL ||
        (got = fread(image, 1u, (size_t)n, f)) != (size_t)n) {
        (void)fclose(f);
        free(image);
        return 1;
    }
    (void)fclose(f);
    memset(ram, 0, RAM_SIZE);
    if (haeun_hxf_load_ex(image, got, ram, RAM_SIZE, &entry, &isa) !=
        HAEUN_HXF_OK) {
        free(image);
        return 1;
    }
    free(image);
    return haeun_vm_init_version(vm, ram, RAM_SIZE, entry, isa) == HAEUN_VM_OK
               ? 0
               : 1;
}

static haeun_eval_result image_path(const haeun_eval_workload *wl,
                                    const char *build_dir, uint32_t policy_id,
                                    char *path, size_t cap) {
    int n;
    if (policy_id == HAEUN_EVAL_POLICY_FCFS)
        n = snprintf(path, cap, "%s/fcfs/%s.hxf", build_dir, wl->image_name);
    else if (policy_id == HAEUN_EVAL_POLICY_RR)
        n = snprintf(path, cap, "%s/%s.hxf", build_dir, wl->image_name);
    else
        return HAEUN_EVAL_BAD_ARGUMENT;
    if (n < 0 || (size_t)n >= cap)
        return HAEUN_EVAL_IO;
    return HAEUN_EVAL_OK;
}

/**
 * Runs one workload repetition against a policy-specific HXF tree.
 */
haeun_eval_result haeun_eval_run_once(const haeun_eval_workload *wl,
                                      const char *build_dir, uint32_t policy_id,
                                      haeun_eval_run_metrics *out) {
    char path[HAEUN_EVAL_PATH_MAX];
    uint8_t *ram;
    haeun_vm vm;
    haeun_vm_result st = HAEUN_VM_OK;
    uint64_t i;
    haeun_eval_result er;
    if (wl == NULL || build_dir == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (image_path(wl, build_dir, policy_id, path, sizeof(path)) != HAEUN_EVAL_OK)
        return HAEUN_EVAL_IO;
    ram = (uint8_t *)calloc(RAM_SIZE, 1u);
    if (ram == NULL)
        return HAEUN_EVAL_IO;
    if (load_hxf(path, &vm, ram) != 0) {
        fprintf(stderr, "haeun-eval: failed to load %s\n", path);
        free(ram);
        out->ok = 0;
        snprintf(out->fail_reason, sizeof(out->fail_reason), "load failed");
        return HAEUN_EVAL_RUN_FAILED;
    }
    for (i = 0; i < wl->step_cap; ++i) {
        st = haeun_vm_step(&vm);
        if (st == HAEUN_VM_HALTED)
            break;
        if (st != HAEUN_VM_OK) {
            free(ram);
            out->ok = 0;
            snprintf(out->fail_reason, sizeof(out->fail_reason), "vm step error");
            return HAEUN_EVAL_RUN_FAILED;
        }
    }
    if (st != HAEUN_VM_HALTED) {
        free(ram);
        out->ok = 0;
        snprintf(out->fail_reason, sizeof(out->fail_reason), "step cap exceeded");
        return HAEUN_EVAL_RUN_FAILED;
    }
    if (vm.r[1] != wl->expected_exit) {
        free(ram);
        out->ok = 0;
        snprintf(out->fail_reason, sizeof(out->fail_reason),
                 "exit %u != expected %u", (unsigned)vm.r[1],
                 (unsigned)wl->expected_exit);
        return HAEUN_EVAL_RUN_FAILED;
    }
    if (rd32(ram, POL_OFF) != policy_id) {
        free(ram);
        out->ok = 0;
        snprintf(out->fail_reason, sizeof(out->fail_reason),
                 "policy id mismatch");
        return HAEUN_EVAL_RUN_FAILED;
    }
    er = haeun_eval_metrics_from_vm(&vm, vm.r[1], i + 1u, out);
    free(ram);
    if (er != HAEUN_EVAL_OK)
        return er;
    if (out->participants < wl->expected_min_pids) {
        out->ok = 0;
        snprintf(out->fail_reason, sizeof(out->fail_reason),
                 "participants %u < min %u", out->participants,
                 wl->expected_min_pids);
        return HAEUN_EVAL_RUN_FAILED;
    }
    return HAEUN_EVAL_OK;
}

/**
 * Executes requested_reps independent runs and fills the experiment.
 */
haeun_eval_result haeun_eval_experiment_run(const haeun_eval_workload *wl,
                                            const char *build_dir,
                                            uint32_t policy_id,
                                            const char *experiment_id,
                                            unsigned requested_reps,
                                            haeun_eval_experiment *out) {
    unsigned r;
    unsigned reps = requested_reps;
    if (wl == NULL || build_dir == NULL || experiment_id == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    if (reps == 0u)
        reps = HAEUN_EVAL_DEFAULT_REPS;
    if (reps > HAEUN_EVAL_MAX_RUNS)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->workload = *wl;
    out->policy_id = policy_id;
    out->requested_reps = reps;
    snprintf(out->experiment_id, sizeof(out->experiment_id), "%s",
             experiment_id);
    snprintf(out->build_id, sizeof(out->build_id), "%s", HAEUN_EVAL_BUILD_ID);
    if (policy_id == HAEUN_EVAL_POLICY_FCFS)
        snprintf(out->policy_name, sizeof(out->policy_name), "fcfs_preemptive_fifo");
    else
        snprintf(out->policy_name, sizeof(out->policy_name), "rr");
    for (r = 0; r < reps; ++r) {
        haeun_eval_result er;
        out->runs[r].index = r;
        er = haeun_eval_run_once(wl, build_dir, policy_id, &out->runs[r].metrics);
        if (er == HAEUN_EVAL_OK && out->runs[r].metrics.ok)
            out->success_count++;
        else
            out->fail_count++;
    }
    fprintf(stderr, "haeun-eval: experiment %s policy=%s ok=%u fail=%u\n",
            out->experiment_id, out->policy_name, out->success_count,
            out->fail_count);
    return HAEUN_EVAL_OK;
}
