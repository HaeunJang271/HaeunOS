/**
 * R3.0 host FCFS mirror: ordering, ties, rollover, invalid ID, determinism.
 */
#include "haeun/policy_verify.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int test_order_and_tie(void) {
    haeun_policy_fixture fx;
    uint32_t a = 0u, b = 0u;
    printf("RUN order_tie\n");
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_FCFS;
    fx.state[0] = 1u;
    fx.state[1] = 1u;
    fx.state[2] = 3u;
    fx.ready_since[0] = 100u;
    fx.ready_since[1] = 50u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1);
    CHECK(a == 1u);
    fx.ready_since[0] = 50u;
    fx.ready_since[1] = 50u;
    CHECK(haeun_policy_select_host(&fx, &a) == 1);
    CHECK(a == 0u);
    CHECK(haeun_policy_select_host(&fx, &b) == 1 && b == a);
    printf("order_tie ok\n");
    return 0;
}

static int test_rollover_and_empty(void) {
    haeun_policy_fixture fx;
    uint32_t slot = 0u;
    printf("RUN rollover_empty\n");
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_FCFS;
    fx.state[0] = 1u;
    fx.state[1] = 1u;
    fx.ready_since[0] = 0xFFFFFFF0u;
    fx.ready_since[1] = 0x00000010u;
    /* signed(0xFFFFFFF0 - 0x10) < 0 ⇒ slot0 older within half-range. */
    CHECK(haeun_policy_select_host(&fx, &slot) == 1);
    CHECK(slot == 0u);
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_FCFS;
    fx.state[0] = 3u;
    fx.state[1] = 4u;
    fx.state[2] = 5u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 0);
    fx.policy_id = 99u;
    fx.state[0] = 1u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 0);
    printf("rollover_empty ok\n");
    return 0;
}

static int test_rr_unchanged(void) {
    haeun_policy_fixture fx;
    uint32_t slot = 0u;
    printf("RUN rr_unchanged\n");
    memset(&fx, 0, sizeof(fx));
    fx.policy_id = HAEUN_POLICY_RR;
    fx.curr = 0u;
    fx.state[0] = 1u;
    fx.state[1] = 3u;
    fx.state[2] = 1u;
    CHECK(haeun_policy_select_host(&fx, &slot) == 1);
    CHECK(slot == 2u);
    printf("rr_unchanged ok\n");
    return 0;
}

/**
 * Entry for R3.0 host FCFS unit tests.
 */
int main(void) {
    if (test_order_and_tie() || test_rollover_and_empty() || test_rr_unchanged())
        return 1;
    printf("r30 fcfs unit tests passed\n");
    return 0;
}
