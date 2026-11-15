#ifndef HAEUN_VM_NET_HOST_H
#define HAEUN_VM_NET_HOST_H

#include "haeun/ecall_mux.h"
#include "haeun/net.h"
#include "haeun/net_backend.h"
#include "haeun/net_sock.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/vm.h"

#include <stdint.h>

enum {
    /* Avoid process-v1 page tables at 0x2000/0x3000 (and sibling slots). */
    HAEUN_NET_HOST_TX_PA = 0x0000B000u,
    HAEUN_NET_HOST_RX_PA = 0x0000C000u,
    HAEUN_NET_HOST_RX_CAP = 512u
};

typedef struct {
    uint8_t buf[HAEUN_NET_FRAME_MAX];
    uint32_t len;
    int count;
} haeun_net_tx_cap;

typedef struct {
    haeun_net_if nif;
    haeun_net_socks ss;
    haeun_sock_ecall_ctx sock_ctx;
    haeun_ecall_mux_ctx mux;
    haeun_net_tx_cap tx;
    haeun_net_backend be;
    uint8_t mac[6];
    uint8_t peer_mac[6];
    int attached;
    int injected;
    int real_mode;
} haeun_vm_net_host;

/**
 * Binds NIC DMA, installs mux→socket assist, and arms TX capture (loopback).
 * Does not claim a real host NIC or internet access.
 */
haeun_net_result haeun_vm_net_host_attach(haeun_vm_net_host *host,
                                          haeun_vm *vm);

/**
 * Like attach; real_mode!=0 enables the user-mode UDP host backend.
 */
haeun_net_result haeun_vm_net_host_attach_ex(haeun_vm_net_host *host,
                                             haeun_vm *vm, int real_mode);

/** Shuts down the host backend and clears attachment flags (idempotent). */
void haeun_vm_net_host_shutdown(haeun_vm_net_host *host);

/**
 * Steps the guest. Loopback injects one UDP reply after first TX; real mode
 * forwards TX via the host socket and injects polled RX frames.
 */
haeun_vm_result haeun_vm_net_host_run(haeun_vm_net_host *host, haeun_vm *vm,
                                      uint64_t max_steps,
                                      uint64_t *steps_executed);

#endif
