#include "haeun/board_mmio.h"
#include "haeun/board_net.h"
#include "haeun/net.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-drv] security/validation failure: %s\n", what);
}

/**
 * Initializes a network interface with local addressing.
 * ipv4_host is a.b.c.d packed as (a<<24)|(b<<16)|(c<<8)|d.
 */
haeun_net_result haeun_net_if_init(haeun_net_if *nif, haeun_vm *vm,
                                   const uint8_t mac[6], uint32_t ipv4_host) {
    if (nif == NULL || vm == NULL || mac == NULL) {
        log_fail("if_init bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    memset(nif, 0, sizeof(*nif));
    nif->vm = vm;
    memcpy(nif->mac, mac, 6u);
    nif->ipv4_be = ipv4_host;
    return HAEUN_NET_OK;
}

/**
 * Programs NIC registers for DMA and enables the adapter.
 */
haeun_net_result haeun_net_if_bind(haeun_net_if *nif, uint32_t tx_pa,
                                   uint32_t rx_pa, uint32_t rx_cap) {
    uint32_t mac0, mac1;
    uint64_t end;
    if (nif == NULL || nif->vm == NULL || rx_cap < 42u ||
        rx_cap > HAEUN_NET_FRAME_MAX) {
        log_fail("if_bind bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    end = (uint64_t)rx_pa + (uint64_t)rx_cap;
    if (end < (uint64_t)rx_pa || end > (uint64_t)nif->vm->ram_size) {
        log_fail("if_bind RX DMA OOB");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    end = (uint64_t)tx_pa + (uint64_t)HAEUN_NET_FRAME_MAX;
    if (end < (uint64_t)tx_pa || end > (uint64_t)nif->vm->ram_size) {
        log_fail("if_bind TX DMA OOB");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    nif->tx_pa = tx_pa;
    nif->rx_pa = rx_pa;
    nif->rx_cap = rx_cap;
    mac0 = (uint32_t)nif->mac[0] | ((uint32_t)nif->mac[1] << 8) |
           ((uint32_t)nif->mac[2] << 16) | ((uint32_t)nif->mac[3] << 24);
    mac1 = (uint32_t)nif->mac[4] | ((uint32_t)nif->mac[5] << 8);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 8u, mac0);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 12u, mac1);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 16u, tx_pa);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 28u, rx_pa);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 32u, rx_cap);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 4u, 3u); /* enable + RX IRQ */
    nif->bound = 1;
    return HAEUN_NET_OK;
}

/**
 * Copies a frame into guest TX DMA and kicks the NIC.
 */
haeun_net_result haeun_net_tx_frame(haeun_net_if *nif, const uint8_t *frame,
                                    uint32_t length) {
    if (nif == NULL || !nif->bound || frame == NULL || length == 0u ||
        length > HAEUN_NET_FRAME_MAX) {
        log_fail("tx_frame bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    memcpy(nif->vm->ram + nif->tx_pa, frame, length);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 16u, nif->tx_pa);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 20u, length);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 24u, 1u);
    return HAEUN_NET_OK;
}

/**
 * Processes one pending RX frame from guest DMA (ARP/ICMP/UDP).
 */
haeun_net_result haeun_net_poll(haeun_net_if *nif) {
    uint32_t status;
    uint32_t len;
    const uint8_t *frame;
    uint8_t reply[HAEUN_NET_FRAME_MAX];
    uint32_t reply_len = 0u;
    haeun_net_result ar;
    if (nif == NULL || !nif->bound) {
        log_fail("poll not bound");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    status = haeun_board_read(nif->vm, HAEUN_NET_BASE);
    if ((status & 1u) == 0u) return HAEUN_NET_DROP;
    len = haeun_board_read(nif->vm, HAEUN_NET_BASE + 36u);
    if (len < 42u || len > nif->rx_cap) {
        log_fail("poll bad RX_LEN");
        haeun_board_write(nif->vm, HAEUN_NET_BASE + 40u, 1u);
        return HAEUN_NET_DROP;
    }
    frame = nif->vm->ram + nif->rx_pa;
    ar = haeun_net_arp_reply(nif, frame, len, reply, sizeof(reply), &reply_len);
    if (ar == HAEUN_NET_DROP)
        ar = haeun_net_icmp_echo_reply(nif, frame, len, reply, sizeof(reply),
                                       &reply_len);
    if (ar == HAEUN_NET_DROP) ar = haeun_net_udp_input(nif, frame, len);
    if (ar == HAEUN_NET_DROP)
        ar = haeun_net_tcp_input(nif, frame, len, reply, sizeof(reply),
                                 &reply_len);
    haeun_board_write(nif->vm, HAEUN_NET_BASE + 40u, 1u);
    if (ar == HAEUN_NET_OK && reply_len != 0u)
        return haeun_net_tx_frame(nif, reply, reply_len);
    if (ar == HAEUN_NET_OK && nif->dns_enabled && nif->udp_pending)
        return haeun_net_dns_serve(nif);
    return ar;
}
