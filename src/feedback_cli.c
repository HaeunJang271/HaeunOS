#include "haeun/feedback.h"

#include "haeun/planner.h"

#include <stdio.h>
#include <string.h>

/**
 * CLI entry: report / adaptive cycles / dry-run / optional state file.
 */
int haeun_fb_cli_run(haeun_vm *vm, int json, int adaptive, int dry_run,
                     unsigned cycles, const char *state_path) {
    haeun_fb_state st;
    haeun_plan_session sess;
    haeun_plan plan;
    haeun_fb_assessment assess;
    unsigned c;
    FILE *sf;
    if (vm == NULL)
        return 2;
    haeun_fb_state_init(&st);
    haeun_plan_session_init(&sess);
    if (state_path != NULL) {
        sf = fopen(state_path, "rb");
        if (sf != NULL) {
            if (haeun_fb_state_load(sf, &st) != HAEUN_FB_OK) {
                (void)fclose(sf);
                fprintf(stderr, "feedback: failed to load state %s\n",
                        state_path);
                return 1;
            }
            (void)fclose(sf);
        }
    }
    if (adaptive)
        st.mode = HAEUN_FB_MODE_ADAPTIVE;
    else
        st.mode = HAEUN_FB_MODE_SHADOW;
    if (cycles == 0u)
        cycles = 1u;
    memset(&assess, 0, sizeof(assess));
    memset(&plan, 0, sizeof(plan));
    for (c = 0; c < cycles; ++c) {
        int exec = adaptive && !dry_run;
        if (haeun_fb_cycle(&st, &sess, vm, exec, &plan, &assess) != HAEUN_FB_OK) {
            fprintf(stderr, "feedback: cycle %u failed\n", c);
            return 1;
        }
    }
    if (json)
        haeun_fb_fprint_json(stdout, &st, &assess, &plan);
    else
        haeun_fb_fprint(stdout, &st, &assess, &plan);
    if (state_path != NULL) {
        sf = fopen(state_path, "wb");
        if (sf == NULL) {
            fprintf(stderr, "feedback: cannot write state %s\n", state_path);
            return 1;
        }
        if (haeun_fb_state_save(sf, &st) != HAEUN_FB_OK) {
            (void)fclose(sf);
            return 1;
        }
        (void)fclose(sf);
    }
    return 0;
}
