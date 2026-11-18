#include "haeun/optimize.h"

#include "haeun/planner.h"

#include <stdio.h>
#include <string.h>

/**
 * CLI: optimize / report / apply. Returns process exit code.
 */
int haeun_opt_cli_run(haeun_vm *vm, const char *build_dir, int json,
                      int optimize, int dry_run, int apply,
                      const char *profile_name, unsigned reps,
                      const char *state_path, const char *apply_path) {
    haeun_opt_state st;
    haeun_opt_profile_id pid = HAEUN_OPT_PROF_BALANCED;
    FILE *sf;
    if (vm == NULL)
        return 2;
    haeun_opt_state_init(&st);
    if (build_dir != NULL)
        snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s", build_dir);
    if (profile_name != NULL &&
        haeun_opt_profile_parse(profile_name, &pid) != 0) {
        fprintf(stderr, "optimize: unknown profile %s\n", profile_name);
        return 2;
    }
    st.cfg.profile_id = pid;
    (void)haeun_opt_profile_get(pid, &st.profile);
    if (reps > 0u)
        st.cfg.reps = reps;
    st.cfg.config_fingerprint =
        (uint32_t)st.cfg.profile_id * 1000u + st.cfg.reps * 10u +
        st.cfg.min_success;
    if (state_path != NULL && !apply) {
        sf = fopen(state_path, "rb");
        if (sf != NULL) {
            if (haeun_opt_state_load(sf, &st) != HAEUN_OPT_OK) {
                (void)fclose(sf);
                fprintf(stderr, "optimize: bad state file\n");
                return 1;
            }
            (void)fclose(sf);
            st.cfg.profile_id = pid;
            (void)haeun_opt_profile_get(pid, &st.profile);
            if (reps > 0u)
                st.cfg.reps = reps;
            if (build_dir != NULL)
                snprintf(st.cfg.build_dir, sizeof(st.cfg.build_dir), "%s",
                         build_dir);
        }
    }
    if (apply) {
        haeun_plan plan;
        haeun_opt_recommendation rec;
        st.mode = HAEUN_OPT_MODE_APPLY;
        if (apply_path != NULL) {
            sf = fopen(apply_path, "rb");
            if (sf == NULL || haeun_opt_state_load(sf, &st) != HAEUN_OPT_OK) {
                if (sf)
                    (void)fclose(sf);
                fprintf(stderr, "optimize: cannot load recommendation file\n");
                return 1;
            }
            (void)fclose(sf);
        }
        rec = st.last_rec;
        if (!rec.valid || rec.decision != HAEUN_OPT_DEC_SELECT) {
            fprintf(stderr, "optimize: no valid SELECT recommendation\n");
            return 1;
        }
        if (dry_run) {
            fprintf(stdout, "APPLY dry-run: would target policy %u (%s)\n",
                    rec.recommended_policy, rec.experiment_id);
            return 0;
        }
        if (haeun_opt_apply(vm, &rec, &plan) != HAEUN_OPT_OK) {
            fprintf(stderr, "optimize: APPLY rejected/failed\n");
            return 1;
        }
        fprintf(stdout, "APPLY committed=%d txn=%s policy=%u\n",
                plan.txn_committed, plan.txn_id, rec.recommended_policy);
        return 0;
    }
    if (optimize) {
        st.mode = dry_run ? HAEUN_OPT_MODE_SHADOW : HAEUN_OPT_MODE_OPTIMIZE;
        /* OPTIMIZE explores offline; never mutates active policy here. */
        if (haeun_opt_run(&st) != HAEUN_OPT_OK) {
            fprintf(stderr, "optimize: run failed\n");
            return 1;
        }
    } else {
        st.mode = HAEUN_OPT_MODE_SHADOW;
    }
    if (json)
        haeun_opt_fprint_json(stdout, &st);
    else
        haeun_opt_fprint(stdout, &st);
    if (state_path != NULL) {
        sf = fopen(state_path, "wb");
        if (sf == NULL || haeun_opt_state_save(sf, &st) != HAEUN_OPT_OK) {
            if (sf)
                (void)fclose(sf);
            return 1;
        }
        (void)fclose(sf);
    }
    (void)dry_run;
    return 0;
}
