#include "haeun/reconfig.h"

#include <stdio.h>

static haeun_recfg_fi_point g_fi = HAEUN_RECFG_FI_NONE;

/**
 * Clears failure-injection point (test-only; default NONE).
 */
void haeun_recfg_fi_clear(void) {
    g_fi = HAEUN_RECFG_FI_NONE;
}

/**
 * Arms a test-only failure injection point.
 */
void haeun_recfg_fi_arm(haeun_recfg_fi_point point) {
    g_fi = point;
    fprintf(stderr, "haeun-recfg: FI armed point=%d\n", (int)point);
}

/** Returns 1 if the armed point matches and then clears it. */
int haeun_recfg_fi_hit(haeun_recfg_fi_point point) {
    if (g_fi != point)
        return 0;
    g_fi = HAEUN_RECFG_FI_NONE;
    fprintf(stderr, "haeun-recfg: FI hit point=%d\n", (int)point);
    return 1;
}
