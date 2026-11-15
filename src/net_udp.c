#include "haeun/net.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-udp] security/validation failure: %s\n", what);
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

static uint16_t udp_checksum(uint32_t src_ip, uint32_t dst_ip,
                             const uint8_t *udp, uint32_t udp_len) {
    uint8_t pseudo[12];
    uint32_t sum = 0u;
    uint32_t i;
    write_u32(pseudo, src_ip);
    write_u32(pseudo + 4, dst_ip);
    pseudo[8] = 0u;
    pseudo[9] = HAEUN_IP_PROTO_UDP;
    write_u16(pseudo + 10, (uint16_t)udp_len);
    for (i = 0u; i < 12u; i += 2u)
        sum += ((uint32_t)pseudo[i] << 8) | (uint32_t)pseudo[i + 1u];
    for (i = 0u; i + 1u < udp_len; i += 2u)
        sum += ((uint32_t)udp[i] << 8) | (uint32_t)udp[i + 1u];
    if ((udp_len & 1u) != 0u) sum += (uint32_t)udp[udp_len - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xffffu) + (sum >> 16);
    sum = ~sum & 0xffffu;
    return sum == 0u ? 0xffffu : (uint16_t)sum;
}

/**
 * Binds the interface to accept UDP datagrams on port.
 */
haeun_net_result haeun_net_udp_bind(haeun_net_if *nif, uint16_t port) {
    if (nif == NULL || !nif->bound || port == 0u) {
        log_fail("udp_bind bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    nif->udp_port = port;
    nif->udp_pending = 0;
    return HAEUN_NET_OK;
}

/**
 * Builds and transmits one UDP datagram through the NIC.
 */
haeun_net_result haeun_net_udp_send(haeun_net_if *nif, const uint8_t dst_mac[6],
                                    uint32_t dst_ip, uint16_t dst_port,
                                    uint16_t src_port, const uint8_t *data,
                                    uint32_t length) {
    uint8_t frame[HAEUN_NET_FRAME_MAX];
    uint8_t *ip;
    uint8_t *udp;
    uint16_t udp_len;
    uint16_t total;
    uint32_t frame_len;
    if (nif == NULL || !nif->bound || dst_mac == NULL || src_port == 0u ||
        dst_port == 0u || (data == NULL && length != 0u) ||
        length > HAEUN_NET_UDP_MAX) {
        log_fail("udp_send bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    udp_len = (uint16_t)(HAEUN_UDP_HDR_LEN + length);
    total = (uint16_t)(HAEUN_IPV4_HDR_LEN + udp_len);
    frame_len = HAEUN_ETH_HDR_LEN + (uint32_t)total;
    if (frame_len > HAEUN_NET_FRAME_MAX) return HAEUN_NET_NOSPACE;
    memset(frame, 0, frame_len);
    haeun_net_eth_write_hdr(frame, dst_mac, nif->mac, HAEUN_ETH_TYPE_IPV4);
    ip = frame + HAEUN_ETH_HDR_LEN;
    ip[0] = 0x45u;
    write_u16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_UDP;
    write_u32(ip + 12, nif->ipv4_be);
    write_u32(ip + 16, dst_ip);
    write_u16(ip + 10, haeun_net_checksum(ip, HAEUN_IPV4_HDR_LEN));
    udp = ip + HAEUN_IPV4_HDR_LEN;
    write_u16(udp, src_port);
    write_u16(udp + 2, dst_port);
    write_u16(udp + 4, udp_len);
    if (length != 0u) memcpy(udp + 8, data, length);
    write_u16(udp + 6, udp_checksum(nif->ipv4_be, dst_ip, udp, udp_len));
    return haeun_net_tx_frame(nif, frame, frame_len);
}

/**
 * Delivers a pending UDP payload to the caller.
 */
haeun_net_result haeun_net_udp_recv(haeun_net_if *nif, uint32_t *src_ip,
                                    uint16_t *src_port, uint8_t *out,
                                    uint32_t out_cap, uint32_t *out_len) {
    if (nif == NULL || src_ip == NULL || src_port == NULL || out == NULL ||
        out_len == NULL) {
        log_fail("udp_recv bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (!nif->udp_pending) return HAEUN_NET_DROP;
    if (nif->udp_payload_len > out_cap) {
        log_fail("udp_recv buffer too small");
        return HAEUN_NET_NOSPACE;
    }
    memcpy(out, nif->udp_payload, nif->udp_payload_len);
    *out_len = nif->udp_payload_len;
    *src_ip = nif->udp_src_ip;
    *src_port = nif->udp_src_port;
    nif->udp_pending = 0;
    return HAEUN_NET_OK;
}

/**
 * Parses an inbound UDP frame destined to the bound port into the pending slot.
 */
haeun_net_result haeun_net_udp_input(haeun_net_if *nif, const uint8_t *frame,
                                     uint32_t length) {
    const uint8_t *ip;
    const uint8_t *udp;
    uint8_t ihl;
    uint16_t total;
    uint16_t udp_len;
    uint32_t dst_ip;
    uint16_t dst_port;
    uint16_t got_csum;
    uint8_t tmp[HAEUN_NET_FRAME_MAX];
    uint32_t payload_len;
    if (nif == NULL || frame == NULL) {
        log_fail("udp_input bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (nif->udp_port == 0u || nif->udp_pending) return HAEUN_NET_DROP;
    if (haeun_net_eth_type(frame, length) != HAEUN_ETH_TYPE_IPV4)
        return HAEUN_NET_DROP;
    if (length < HAEUN_ETH_HDR_LEN + HAEUN_IPV4_HDR_LEN + HAEUN_UDP_HDR_LEN)
        return HAEUN_NET_DROP;
    ip = frame + HAEUN_ETH_HDR_LEN;
    ihl = (uint8_t)((ip[0] & 0x0fu) * 4u);
    if ((ip[0] >> 4) != 4u || ihl != HAEUN_IPV4_HDR_LEN ||
        ip[9] != HAEUN_IP_PROTO_UDP)
        return HAEUN_NET_DROP;
    total = read_u16(ip + 2);
    if ((uint32_t)HAEUN_ETH_HDR_LEN + (uint32_t)total > length ||
        total < ihl + HAEUN_UDP_HDR_LEN) {
        log_fail("udp_input bad IP length");
        return HAEUN_NET_DROP;
    }
    {
        uint8_t hdr[20];
        memcpy(hdr, ip, 20u);
        hdr[10] = 0u;
        hdr[11] = 0u;
        if (haeun_net_checksum(hdr, 20u) != read_u16(ip + 10)) {
            log_fail("udp_input IP checksum");
            return HAEUN_NET_DROP;
        }
    }
    dst_ip = read_u32(ip + 16);
    if (dst_ip != nif->ipv4_be) return HAEUN_NET_DROP;
    udp = ip + ihl;
    udp_len = read_u16(udp + 4);
    if (udp_len < HAEUN_UDP_HDR_LEN ||
        (uint32_t)ihl + (uint32_t)udp_len > (uint32_t)total) {
        log_fail("udp_input bad UDP length");
        return HAEUN_NET_DROP;
    }
    dst_port = read_u16(udp + 2);
    if (dst_port != nif->udp_port) return HAEUN_NET_DROP;
    got_csum = read_u16(udp + 6);
    if (got_csum != 0u) {
        if (udp_len > HAEUN_NET_FRAME_MAX) return HAEUN_NET_DROP;
        memcpy(tmp, udp, udp_len);
        tmp[6] = 0u;
        tmp[7] = 0u;
        if (udp_checksum(read_u32(ip + 12), dst_ip, tmp, udp_len) != got_csum) {
            log_fail("udp_input UDP checksum");
            return HAEUN_NET_DROP;
        }
    }
    payload_len = (uint32_t)udp_len - HAEUN_UDP_HDR_LEN;
    if (payload_len > HAEUN_NET_UDP_MAX) {
        log_fail("udp_input payload too large");
        return HAEUN_NET_NOSPACE;
    }
    memcpy(nif->udp_payload, udp + 8, payload_len);
    nif->udp_payload_len = payload_len;
    nif->udp_src_ip = read_u32(ip + 12);
    nif->udp_src_port = read_u16(udp);
    memcpy(nif->udp_src_mac, frame + 6, 6u);
    nif->udp_pending = 1;
    return HAEUN_NET_OK;
}
