#include "haeun/robust.h"

#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

/**
 * CLI entry for R3.7 robustness validation.
 */
int haeun_rb_cli_run(haeun_vm *vm, const char *build_dir, int json, int run,
                     int dry_run, int sensitivity, int stress, int verify,
                     const char *preset_name, unsigned reps,
                     const char *profile_name, const char *state_path,
                     const char *out_dir) {
    haeun_rb_state st;
    haeun_rb_preset preset = HAEUN_RB_PRESET_SMOKE;
    haeun_opt_profile_id pid = HAEUN_OPT_PROF_BALANCED;
    FILE *sf;
    haeun_rb_state_init(&st);
    if (build_dir != NULL)
        snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build_dir);
    if (preset_name != NULL && haeun_rb_preset_parse(preset_name, &preset) != 0) {
        fprintf(stderr, "robustness: unknown preset\n");
        return 2;
    }
    (void)haeun_rb_preset_apply(&st.cfg, preset);
    if (reps > 0u) {
        st.cfg.reps = reps;
        if (st.cfg.min_success > reps)
            st.cfg.min_success = reps ? reps : 1u;
        st.cfg.config_hash = haeun_rb_config_hash(&st.cfg);
    }
    if (profile_name != NULL &&
        haeun_opt_profile_parse(profile_name, &pid) != 0) {
        fprintf(stderr, "robustness: unknown profile\n");
        return 2;
    }
    st.cfg.profile_id = pid;
    st.cfg.config_hash = haeun_rb_config_hash(&st.cfg);
    if (!run && !sensitivity && !stress && !verify) {
        /* report-only on empty state */
        if (json)
            haeun_rb_fprint_json(stdout, &st);
        else
            haeun_rb_fprint(stdout, &st);
        return 0;
    }
    if (run || sensitivity || verify) {
        if (haeun_rb_run_experiments(&st) != HAEUN_RB_OK) {
            fprintf(stderr, "robustness: experiment run failed\n");
            return 1;
        }
    }
    if (sensitivity) {
        if (haeun_rb_run_sensitivity(&st) != HAEUN_RB_OK)
            return 1;
        (void)haeun_rb_workload_robustness(&st);
    }
    if (verify)
        (void)haeun_rb_verify_recommendations(&st);
    if (stress)
        (void)haeun_rb_reconfig_stress(&st, dry_run ? NULL : vm, dry_run);
    haeun_rb_aggregate(&st);
    if (json)
        haeun_rb_fprint_json(stdout, &st);
    else
        haeun_rb_fprint(stdout, &st);
    if (out_dir != NULL && haeun_rb_export(&st, out_dir) != HAEUN_RB_OK) {
        fprintf(stderr, "robustness: export failed\n");
        return 1;
    }
    if (state_path != NULL) {
        sf = fopen(state_path, "wb");
        if (sf == NULL || haeun_rb_state_save(sf, &st) != HAEUN_RB_OK) {
            if (sf)
                (void)fclose(sf);
            return 1;
        }
        (void)fclose(sf);
    }
    if (st.overall == HAEUN_RB_OVERALL_FAIL)
        return 1;
    return 0;
}
