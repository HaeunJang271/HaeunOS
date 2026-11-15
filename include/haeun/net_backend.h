#ifndef HAEUN_NET_BACKEND_H
#define HAEUN_NET_BACKEND_H

#include "haeun/net.h"

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET haeun_be_sock;
#define HAEUN_BE_INVALID_SOCK INVALID_SOCKET
#else
typedef int haeun_be_sock;
#define HAEUN_BE_INVALID_SOCK (-1)
#endif

enum {
    HAEUN_NET_BE_OK = 0,
    HAEUN_NET_BE_BAD_ARGUMENT = 1,
    HAEUN_NET_BE_IO = 2,
    HAEUN_NET_BE_DROP = 3,
    HAEUN_NET_BE_EMPTY = 4
};

/**
 * User-mode UDP socket backend: strips guest Ethernet/IPv4/UDP and forwards
 * via a host datagram socket. Does not expose host fds to the guest.
 */
typedef struct {
    int ready;
    haeun_be_sock sock;
    uint8_t guest_mac[6];
    uint8_t peer_mac[6];
    uint32_t guest_ip;
    uint32_t last_guest_ip;
    uint16_t last_guest_port;
    uint32_t tx_ok;
    uint32_t rx_ok;
    uint32_t drops;
    uint32_t io_errors;
} haeun_net_backend;

/**
 * Opens a non-blocking host UDP socket and records guest L2/L3 identity.
 */
int haeun_net_backend_init(haeun_net_backend *be, const uint8_t guest_mac[6],
                           const uint8_t peer_mac[6], uint32_t guest_ip);

/** Closes the host socket and clears state (idempotent). */
void haeun_net_backend_shutdown(haeun_net_backend *be);

/**
 * Parses a guest TX Ethernet frame; forwards IPv4/UDP via the host socket.
 * Malformed or non-UDP frames are dropped and counted.
 */
int haeun_net_backend_on_tx(haeun_net_backend *be, const uint8_t *frame,
                            uint32_t length);

/**
 * Non-blocking host recv; on success writes a checksummed Ethernet reply
 * destined to the guest. Returns HAEUN_NET_BE_EMPTY when no datagram is ready.
 */
int haeun_net_backend_poll_rx(haeun_net_backend *be, uint8_t *out,
                              uint32_t out_cap, uint32_t *out_len);

#endif
