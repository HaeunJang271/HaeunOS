/**
 * Shared helpers for M20.8 host-side integration / security unit tests.
 */
#ifndef M20_INTEGRATION_HARNESS_H
#define M20_INTEGRATION_HARNESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum { M20I_PCB_BASE = 0xA000u, M20I_PCB_STRIDE = 0x100u, M20I_PCB_NEXT = 0xA404u };

void m20i_wr32(uint8_t *p, uint32_t v);
void m20i_prep_pcb(uint8_t *ram, size_t n);
void m20i_make_tiny_hxf(uint8_t *out, size_t *out_len);

int m20i_test_abi_map(void);
int m20i_test_ownership_isolation(void);
int m20i_test_resource_exhaust_recover(void);
int m20i_test_hfs_repeat_and_remount(void);
int m20i_test_evt_overflow_repeat(void);
int m20i_test_service_repeat(void);
int m20i_test_sock_ipc_fd_security(void);
int m20i_test_repeat_win_ipc_sock(void);

#endif
