#include "haeun/hxf.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * User privilege cannot pass the supervisor gate; supervisor can.
 */
static int test_privilege_gate(void) {
    CHECK(haeun_sec_require_supervisor(HAEUN_PRIV_USER) == HAEUN_SEC_DENIED);
    CHECK(haeun_sec_require_supervisor(HAEUN_PRIV_SUPERVISOR) == HAEUN_SEC_OK);
    return 0;
}

/**
 * Known syscalls are allowed; out-of-range numbers are denied and logged.
 */
static int test_syscall_validation(void) {
    CHECK(haeun_sec_validate_syscall(HAEUN_PRIV_USER, HAEUN_SYS_YIELD) ==
          HAEUN_SEC_OK);
    CHECK(haeun_sec_validate_syscall(HAEUN_PRIV_USER, 99u) == HAEUN_SEC_DENIED);
    return 0;
}

/**
 * User pointer windows are enforced; supervisor bypasses.
 */
static int test_user_range(void) {
    CHECK(haeun_sec_check_user_range(HAEUN_PRIV_USER, 0x1000u, 16u, 0x1000u,
                                     0x2000u) == HAEUN_SEC_OK);
    CHECK(haeun_sec_check_user_range(HAEUN_PRIV_USER, 0x1ff0u, 32u, 0x1000u,
                                     0x2000u) == HAEUN_SEC_DENIED);
    CHECK(haeun_sec_check_user_range(HAEUN_PRIV_SUPERVISOR, 0u, 0xffffffffu, 0u,
                                     1u) == HAEUN_SEC_OK);
    return 0;
}

/**
 * HXF magic/ISA/entry checks reject garbage and accept a minimal written image.
 */
static int test_exec_validation(void) {
    uint8_t bad[64];
    uint8_t image[256];
    uint8_t seg[4] = {0x00, 0x00, 0x00, 0x04}; /* HALT encoding style */
    haeun_hxf_segment s;
    size_t out_size = 0u;
    memset(bad, 0, sizeof(bad));
    CHECK(haeun_sec_validate_exec(bad, sizeof(bad)) == HAEUN_SEC_DENIED);
    memset(&s, 0, sizeof(s));
    s.flags = 5u; /* includes executable bit for entry */
    s.virtual_address = 0x1000u;
    s.file_data = seg;
    s.file_size = 4u;
    s.memory_size = 8u;
    s.alignment = 4u;
    CHECK(haeun_hxf_write(&s, 1u, 0x1000u, image, sizeof(image), &out_size) ==
          HAEUN_HXF_OK);
    CHECK(out_size >= 32u);
    CHECK(haeun_sec_validate_exec(image, out_size) == HAEUN_SEC_OK);
    image[8] = 9u; /* corrupt ISA */
    CHECK(haeun_sec_validate_exec(image, out_size) == HAEUN_SEC_DENIED);
    return 0;
}

int main(void) {
    if (test_privilege_gate() != 0) return 1;
    if (test_syscall_validation() != 0) return 1;
    if (test_user_range() != 0) return 1;
    if (test_exec_validation() != 0) return 1;
    puts("all sec tests passed");
    return 0;
}
