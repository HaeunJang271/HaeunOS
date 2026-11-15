#ifndef HAEUN_NET_SOCK_H
#define HAEUN_NET_SOCK_H

#include "haeun/net.h"

#include <stdint.h>

enum {
    HAEUN_SOCK_DGRAM = 1,
    HAEUN_SOCK_STREAM = 2,
    HAEUN_NET_MAX_SOCKS = 4
};

typedef struct {
    int used;
    int type;
    int owner_pid; /* 0 = unowned / non-process context */
    uint16_t local_port;
    int listening;
    int accepted;
    uint32_t peer_ip;
    uint16_t peer_port;
} haeun_sock;

typedef struct {
    haeun_net_if *nif;
    haeun_sock socks[HAEUN_NET_MAX_SOCKS];
    uint16_t ephemeral;
} haeun_net_socks;

/**
 * Initializes an empty socket table bound to a network interface.
 */
haeun_net_result haeun_socks_init(haeun_net_socks *ss, haeun_net_if *nif);

/**
 * Allocates a socket of HAEUN_SOCK_DGRAM or HAEUN_SOCK_STREAM; returns fd in *out_fd.
 */
haeun_net_result haeun_socket(haeun_net_socks *ss, int type, int *out_fd);

/** Binds a socket to a local UDP/TCP port. */
haeun_net_result haeun_sock_bind(haeun_net_socks *ss, int fd, uint16_t port);

/**
 * Sends a UDP datagram through the NIC (dst_mac is the next-hop address).
 */
haeun_net_result haeun_sock_sendto(haeun_net_socks *ss, int fd,
                                   const uint8_t dst_mac[6], uint32_t dst_ip,
                                   uint16_t dst_port, const uint8_t *data,
                                   uint32_t length);

/**
 * Receives a pending UDP datagram for this socket after haeun_net_poll.
 */
haeun_net_result haeun_sock_recvfrom(haeun_net_socks *ss, int fd,
                                     uint32_t *src_ip, uint16_t *src_port,
                                     uint8_t *out, uint32_t out_cap,
                                     uint32_t *out_len);

/** Marks a stream socket as a TCP listener on its bound port. */
haeun_net_result haeun_sock_listen(haeun_net_socks *ss, int fd);

/**
 * After a completed handshake, returns a connected stream fd for the peer.
 */
haeun_net_result haeun_sock_accept(haeun_net_socks *ss, int listen_fd,
                                   int *out_fd, uint32_t *peer_ip,
                                   uint16_t *peer_port);

/** Releases a socket slot. */
haeun_net_result haeun_sock_close(haeun_net_socks *ss, int fd);

/**
 * Sets the owning process pid on an allocated socket (0 clears ownership).
 */
haeun_net_result haeun_sock_set_owner(haeun_net_socks *ss, int fd, int pid);

/**
 * Returns 1 when fd is used and owned by pid.
 * pid==0 matches only unowned sockets (legacy non-process tests).
 */
int haeun_sock_owned_by(const haeun_net_socks *ss, int fd, int pid);

/**
 * Closes every socket whose owner_pid matches pid (pid must be nonzero).
 */
haeun_net_result haeun_socks_close_by_owner(haeun_net_socks *ss, int pid);

#endif
