#include "haeun/net_sock.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-sock] security/validation failure: %s\n", what);
}

static haeun_sock *slot(haeun_net_socks *ss, int fd) {
    if (ss == NULL || fd < 0 || fd >= HAEUN_NET_MAX_SOCKS) return NULL;
    if (!ss->socks[fd].used) return NULL;
    return &ss->socks[fd];
}

/**
 * Clears the socket table and attaches it to nif.
 */
haeun_net_result haeun_socks_init(haeun_net_socks *ss, haeun_net_if *nif) {
    if (ss == NULL || nif == NULL || !nif->bound) {
        log_fail("socks_init bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    memset(ss, 0, sizeof(*ss));
    ss->nif = nif;
    ss->ephemeral = 40000u;
    return HAEUN_NET_OK;
}

/**
 * Allocates one socket descriptor of the requested type.
 */
haeun_net_result haeun_socket(haeun_net_socks *ss, int type, int *out_fd) {
    int i;
    if (ss == NULL || out_fd == NULL ||
        (type != HAEUN_SOCK_DGRAM && type != HAEUN_SOCK_STREAM)) {
        log_fail("socket bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    for (i = 0; i < HAEUN_NET_MAX_SOCKS; ++i) {
        if (!ss->socks[i].used) {
            memset(&ss->socks[i], 0, sizeof(ss->socks[i]));
            ss->socks[i].used = 1;
            ss->socks[i].type = type;
            *out_fd = i;
            return HAEUN_NET_OK;
        }
    }
    log_fail("socket table full");
    return HAEUN_NET_NOSPACE;
}

/**
 * Binds fd to a local port and programs the underlying UDP/TCP listener port.
 */
haeun_net_result haeun_sock_bind(haeun_net_socks *ss, int fd, uint16_t port) {
    haeun_sock *s = slot(ss, fd);
    if (s == NULL || port == 0u) {
        log_fail("bind bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (s->type == HAEUN_SOCK_DGRAM) {
        haeun_net_result r = haeun_net_udp_bind(ss->nif, port);
        if (r != HAEUN_NET_OK) return r;
    }
    s->local_port = port;
    return HAEUN_NET_OK;
}

/**
 * Sends a datagram through the guest NIC stack.
 */
haeun_net_result haeun_sock_sendto(haeun_net_socks *ss, int fd,
                                   const uint8_t dst_mac[6], uint32_t dst_ip,
                                   uint16_t dst_port, const uint8_t *data,
                                   uint32_t length) {
    haeun_sock *s = slot(ss, fd);
    uint16_t sport;
    if (s == NULL || s->type != HAEUN_SOCK_DGRAM || dst_mac == NULL) {
        log_fail("sendto bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    sport = s->local_port;
    if (sport == 0u) {
        sport = ss->ephemeral++;
        if (ss->ephemeral < 40000u) ss->ephemeral = 40000u;
        s->local_port = sport;
        {
            haeun_net_result r = haeun_net_udp_bind(ss->nif, sport);
            if (r != HAEUN_NET_OK) return r;
        }
    }
    return haeun_net_udp_send(ss->nif, dst_mac, dst_ip, dst_port, sport, data,
                              length);
}

/**
 * Copies a pending UDP datagram if it matches this socket's port.
 */
haeun_net_result haeun_sock_recvfrom(haeun_net_socks *ss, int fd,
                                     uint32_t *src_ip, uint16_t *src_port,
                                     uint8_t *out, uint32_t out_cap,
                                     uint32_t *out_len) {
    haeun_sock *s = slot(ss, fd);
    if (s == NULL || s->type != HAEUN_SOCK_DGRAM) {
        log_fail("recvfrom bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (s->local_port == 0u || ss->nif->udp_port != s->local_port) {
        log_fail("recvfrom not bound to active UDP port");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    return haeun_net_udp_recv(ss->nif, src_ip, src_port, out, out_cap, out_len);
}

/**
 * Starts a passive TCP listen on a bound stream socket.
 */
haeun_net_result haeun_sock_listen(haeun_net_socks *ss, int fd) {
    haeun_sock *s = slot(ss, fd);
    haeun_net_result r;
    if (s == NULL || s->type != HAEUN_SOCK_STREAM || s->local_port == 0u) {
        log_fail("listen bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    r = haeun_net_tcp_listen(ss->nif, s->local_port);
    if (r != HAEUN_NET_OK) return r;
    s->listening = 1;
    return HAEUN_NET_OK;
}

/**
 * Accepts one established TCP peer into a new stream socket.
 */
haeun_net_result haeun_sock_accept(haeun_net_socks *ss, int listen_fd,
                                   int *out_fd, uint32_t *peer_ip,
                                   uint16_t *peer_port) {
    haeun_sock *ls = slot(ss, listen_fd);
    int nfd = -1;
    haeun_net_result r;
    if (ls == NULL || !ls->listening || out_fd == NULL || peer_ip == NULL ||
        peer_port == NULL) {
        log_fail("accept bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (haeun_net_tcp_state(ss->nif) != HAEUN_TCP_ESTABLISHED) {
        return HAEUN_NET_DROP;
    }
    r = haeun_socket(ss, HAEUN_SOCK_STREAM, &nfd);
    if (r != HAEUN_NET_OK) return r;
    ss->socks[nfd].accepted = 1;
    ss->socks[nfd].local_port = ls->local_port;
    ss->socks[nfd].peer_ip = ss->nif->tcp_peer_ip;
    ss->socks[nfd].peer_port = ss->nif->tcp_peer_port;
    *out_fd = nfd;
    *peer_ip = ss->socks[nfd].peer_ip;
    *peer_port = ss->socks[nfd].peer_port;
    return HAEUN_NET_OK;
}

/**
 * Frees a socket descriptor.
 */
haeun_net_result haeun_sock_close(haeun_net_socks *ss, int fd) {
    haeun_sock *s = slot(ss, fd);
    if (s == NULL) {
        log_fail("close bad fd");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    memset(s, 0, sizeof(*s));
    return HAEUN_NET_OK;
}

/**
 * Records which guest process owns an open socket.
 */
haeun_net_result haeun_sock_set_owner(haeun_net_socks *ss, int fd, int pid) {
    haeun_sock *s = slot(ss, fd);
    if (s == NULL) {
        log_fail("set_owner bad fd");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    s->owner_pid = pid;
    return HAEUN_NET_OK;
}

/**
 * Returns 1 when fd is used and owned by pid (see header for pid==0 rule).
 */
int haeun_sock_owned_by(const haeun_net_socks *ss, int fd, int pid) {
    const haeun_sock *s;
    if (ss == NULL || fd < 0 || fd >= HAEUN_NET_MAX_SOCKS)
        return 0;
    s = &ss->socks[fd];
    if (!s->used)
        return 0;
    return s->owner_pid == pid;
}

/**
 * Releases all sockets owned by pid (used on process EXIT).
 */
haeun_net_result haeun_socks_close_by_owner(haeun_net_socks *ss, int pid) {
    int i;
    if (ss == NULL || pid == 0) {
        log_fail("close_by_owner bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    for (i = 0; i < HAEUN_NET_MAX_SOCKS; ++i) {
        if (ss->socks[i].used && ss->socks[i].owner_pid == pid)
            memset(&ss->socks[i], 0, sizeof(ss->socks[i]));
    }
    return HAEUN_NET_OK;
}
