#include "haeun/net.h"

#include <stdio.h>
#include <string.h>

enum { TCP_ISS_DEFAULT = 1000u };

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-tcp] security/validation failure: %s\n", what);
}

static void write_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void write_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t read_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip,
                             const uint8_t *tcp, uint32_t tcp_len) {
    uint8_t pseudo[12];
    uint32_t sum = 0u;
    uint32_t i;
    write_u32(pseudo, src_ip);
    write_u32(pseudo + 4, dst_ip);
    pseudo[8] = 0u;
    pseudo[9] = HAEUN_IP_PROTO_TCP;
    write_u16(pseudo + 10, (uint16_t)tcp_len);
    for (i = 0u; i < 12u; i += 2u)
        sum += ((uint32_t)pseudo[i] << 8) | (uint32_t)pseudo[i + 1u];
    for (i = 0u; i + 1u < tcp_len; i += 2u)
        sum += ((uint32_t)tcp[i] << 8) | (uint32_t)tcp[i + 1u];
    if ((tcp_len & 1u) != 0u) sum += (uint32_t)tcp[tcp_len - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xffffu) + (sum >> 16);
    return (uint16_t)~sum;
}

/**
 * Arms a passive TCP listener on port.
 */
haeun_net_result haeun_net_tcp_listen(haeun_net_if *nif, uint16_t port) {
    if (nif == NULL || !nif->bound || port == 0u) {
        log_fail("tcp_listen bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    nif->tcp_port = port;
    nif->tcp_state = HAEUN_TCP_LISTEN;
    nif->tcp_iss = TCP_ISS_DEFAULT;
    nif->tcp_snd_nxt = nif->tcp_iss;
    nif->tcp_rcv_nxt = 0u;
    nif->tcp_peer_ip = 0u;
    nif->tcp_peer_port = 0u;
    memset(nif->tcp_peer_mac, 0, 6u);
    return HAEUN_NET_OK;
}

/**
 * Returns the TCP state machine value for the single connection slot.
 */
haeun_tcp_state haeun_net_tcp_state(const haeun_net_if *nif) {
    if (nif == NULL) return HAEUN_TCP_CLOSED;
    return nif->tcp_state;
}

static int ipv4_ok(const uint8_t *ip, uint16_t total, uint32_t frame_len) {
    uint8_t hdr[20];
    if ((ip[0] >> 4) != 4u || (ip[0] & 0x0fu) != 5u) return 0;
    if ((uint32_t)HAEUN_ETH_HDR_LEN + (uint32_t)total > frame_len) return 0;
    if (total < HAEUN_IPV4_HDR_LEN + HAEUN_TCP_HDR_LEN) return 0;
    memcpy(hdr, ip, 20u);
    hdr[10] = 0u;
    hdr[11] = 0u;
    return haeun_net_checksum(hdr, 20u) == read_u16(ip + 10);
}

static haeun_net_result build_synack(haeun_net_if *nif, const uint8_t *frame,
                                     const uint8_t *ip, const uint8_t *tcp,
                                     uint8_t *out, uint32_t out_cap,
                                     uint32_t *out_len) {
    uint8_t *oip;
    uint8_t *otcp;
    uint32_t peer_seq = read_u32(tcp + 4);
    uint16_t total = HAEUN_IPV4_HDR_LEN + HAEUN_TCP_HDR_LEN;
    if (out_cap < HAEUN_ETH_HDR_LEN + total) {
        log_fail("synack buffer too small");
        return HAEUN_NET_NOSPACE;
    }
    memcpy(nif->tcp_peer_mac, frame + 6, 6u);
    nif->tcp_peer_ip = read_u32(ip + 12);
    nif->tcp_peer_port = read_u16(tcp);
    nif->tcp_rcv_nxt = peer_seq + 1u;
    nif->tcp_snd_nxt = nif->tcp_iss + 1u;
    nif->tcp_state = HAEUN_TCP_SYN_RCVD;
    memset(out, 0, HAEUN_ETH_HDR_LEN + total);
    haeun_net_eth_write_hdr(out, nif->tcp_peer_mac, nif->mac,
                            HAEUN_ETH_TYPE_IPV4);
    oip = out + HAEUN_ETH_HDR_LEN;
    oip[0] = 0x45u;
    write_u16(oip + 2, total);
    oip[8] = 64u;
    oip[9] = HAEUN_IP_PROTO_TCP;
    write_u32(oip + 12, nif->ipv4_be);
    write_u32(oip + 16, nif->tcp_peer_ip);
    write_u16(oip + 10, haeun_net_checksum(oip, HAEUN_IPV4_HDR_LEN));
    otcp = oip + HAEUN_IPV4_HDR_LEN;
    write_u16(otcp, nif->tcp_port);
    write_u16(otcp + 2, nif->tcp_peer_port);
    write_u32(otcp + 4, nif->tcp_iss);
    write_u32(otcp + 8, nif->tcp_rcv_nxt);
    otcp[12] = 0x50u; /* data offset 5 */
    otcp[13] = (uint8_t)(HAEUN_TCP_SYN | HAEUN_TCP_ACK);
    write_u16(otcp + 14, 65535u);
    write_u16(otcp + 16,
              tcp_checksum(nif->ipv4_be, nif->tcp_peer_ip, otcp,
                           HAEUN_TCP_HDR_LEN));
    *out_len = HAEUN_ETH_HDR_LEN + total;
    return HAEUN_NET_OK;
}

/**
 * Processes one inbound TCP segment for the passive listener.
 */
haeun_net_result haeun_net_tcp_input(haeun_net_if *nif, const uint8_t *frame,
                                     uint32_t length, uint8_t *out,
                                     uint32_t out_cap, uint32_t *out_len) {
    const uint8_t *ip;
    const uint8_t *tcp;
    uint16_t total;
    uint8_t flags;
    uint16_t dport;
    uint8_t tmp[HAEUN_TCP_HDR_LEN];
    uint16_t got;
    if (nif == NULL || frame == NULL || out == NULL || out_len == NULL) {
        log_fail("tcp_input bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    *out_len = 0u;
    if (nif->tcp_port == 0u || nif->tcp_state == HAEUN_TCP_CLOSED)
        return HAEUN_NET_DROP;
    if (haeun_net_eth_type(frame, length) != HAEUN_ETH_TYPE_IPV4)
        return HAEUN_NET_DROP;
    if (length < HAEUN_ETH_HDR_LEN + HAEUN_IPV4_HDR_LEN + HAEUN_TCP_HDR_LEN)
        return HAEUN_NET_DROP;
    ip = frame + HAEUN_ETH_HDR_LEN;
    if (ip[9] != HAEUN_IP_PROTO_TCP) return HAEUN_NET_DROP;
    total = read_u16(ip + 2);
    if (!ipv4_ok(ip, total, length)) {
        log_fail("tcp_input bad IPv4");
        return HAEUN_NET_DROP;
    }
    if (read_u32(ip + 16) != nif->ipv4_be) return HAEUN_NET_DROP;
    tcp = ip + HAEUN_IPV4_HDR_LEN;
    if (((tcp[12] >> 4) * 4u) != HAEUN_TCP_HDR_LEN) {
        log_fail("tcp options unsupported");
        return HAEUN_NET_DROP;
    }
    memcpy(tmp, tcp, HAEUN_TCP_HDR_LEN);
    tmp[16] = 0u;
    tmp[17] = 0u;
    got = read_u16(tcp + 16);
    if (tcp_checksum(read_u32(ip + 12), nif->ipv4_be, tmp, HAEUN_TCP_HDR_LEN) !=
        got) {
        log_fail("tcp checksum mismatch");
        return HAEUN_NET_DROP;
    }
    dport = read_u16(tcp + 2);
    if (dport != nif->tcp_port) return HAEUN_NET_DROP;
    flags = tcp[13];
    if (nif->tcp_state == HAEUN_TCP_LISTEN) {
        if ((flags & HAEUN_TCP_SYN) == 0u || (flags & HAEUN_TCP_ACK) != 0u)
            return HAEUN_NET_DROP;
        return build_synack(nif, frame, ip, tcp, out, out_cap, out_len);
    }
    if (nif->tcp_state == HAEUN_TCP_SYN_RCVD) {
        uint32_t ack = read_u32(tcp + 8);
        if ((flags & HAEUN_TCP_ACK) == 0u) return HAEUN_NET_DROP;
        if (read_u32(ip + 12) != nif->tcp_peer_ip) return HAEUN_NET_DROP;
        if (read_u16(tcp) != nif->tcp_peer_port) return HAEUN_NET_DROP;
        if (ack != nif->tcp_snd_nxt) return HAEUN_NET_DROP;
        nif->tcp_state = HAEUN_TCP_ESTABLISHED;
        return HAEUN_NET_OK;
    }
    return HAEUN_NET_DROP;
}
