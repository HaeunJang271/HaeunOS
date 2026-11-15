/**
 * M20.8 System Integration & Security Audit — host unit driver.
 */
#include "m20_integration_harness.h"

#include <stdio.h>

int main(void) {
    if (m20i_test_abi_map() != 0)
        return 1;
    if (m20i_test_ownership_isolation() != 0)
        return 1;
    if (m20i_test_resource_exhaust_recover() != 0)
        return 1;
    if (m20i_test_hfs_repeat_and_remount() != 0)
        return 1;
    if (m20i_test_evt_overflow_repeat() != 0)
        return 1;
    if (m20i_test_service_repeat() != 0)
        return 1;
    if (m20i_test_sock_ipc_fd_security() != 0)
        return 1;
    if (m20i_test_repeat_win_ipc_sock() != 0)
        return 1;
    puts("m20_integration_tests ok");
    return 0;
}
