#include "haeun/net.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-ipv4] security/validation failure: %s\n", what);
}

/**
 * Computes the Internet checksum (RFC 1071) over an even/odd byte buffer.
 */
uint16_t haeun_net_checksum(const uint8_t *data, uint32_t length) {
    uint32_t sum = 0u;
    uint32_t i;
    if (data == NULL) return 0u;
    for (i = 0u; i + 1u < length; i += 2u)
        sum += ((uint32_t)data[i] << 8) | (uint32_t)data[i + 1u];
    if ((length & 1u) != 0u) sum += (uint32_t)data[length - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xffffu) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint32_t read_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
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

static int ipv4_header_ok(const uint8_t *ip, uint8_t ihl, uint16_t total,
                          uint32_t frame_len) {
    uint8_t hdr[20];
    uint16_t expect;
    if ((ip[0] >> 4) != 4u || ihl != HAEUN_IPV4_HDR_LEN) {
        log_fail("unsupported IPv4 header");
        return 0;
    }
    if (total < ihl + 8u ||
        (uint32_t)HAEUN_ETH_HDR_LEN + (uint32_t)total > frame_len) {
        log_fail("bad IPv4 total length");
        return 0;
    }
    memcpy(hdr, ip, ihl);
    hdr[10] = 0u;
    hdr[11] = 0u;
    expect = haeun_net_checksum(hdr, ihl);
    if (expect != (uint16_t)(((uint16_t)ip[10] << 8) | (uint16_t)ip[11])) {
        log_fail("IPv4 checksum mismatch");
        return 0;
    }
    return 1;
}

/**
 * Builds an ICMP echo reply Ethernet frame from a validated echo request.
 */
haeun_net_result haeun_net_icmp_echo_reply(const haeun_net_if *nif,
                                           const uint8_t *frame,
                                           uint32_t length, uint8_t *out,
                                           uint32_t out_cap, uint32_t *out_len) {
    const uint8_t *ip;
    const uint8_t *icmp;
    uint8_t ihl;
    uint16_t total;
    uint32_t dst;
    uint32_t icmp_len;
    uint8_t icmp_tmp[HAEUN_NET_FRAME_MAX];
    uint16_t icmp_check;
    uint8_t *oip;
    uint8_t *oicmp;
    uint32_t src;
    if (nif == NULL || frame == NULL || out == NULL || out_len == NULL) {
        log_fail("icmp_echo bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    *out_len = 0u;
    if (haeun_net_eth_type(frame, length) != HAEUN_ETH_TYPE_IPV4)
        return HAEUN_NET_DROP;
    if (length < HAEUN_ETH_HDR_LEN + HAEUN_IPV4_HDR_LEN + 8u)
        return HAEUN_NET_DROP;
    ip = frame + HAEUN_ETH_HDR_LEN;
    ihl = (uint8_t)((ip[0] & 0x0fu) * 4u);
    total = (uint16_t)(((uint16_t)ip[2] << 8) | (uint16_t)ip[3]);
    if (!ipv4_header_ok(ip, ihl, total, length)) return HAEUN_NET_DROP;
    if (ip[9] != HAEUN_IP_PROTO_ICMP) return HAEUN_NET_DROP;
    dst = read_u32(ip + 16);
    if (dst != nif->ipv4_be) return HAEUN_NET_DROP;
    icmp = ip + ihl;
    icmp_len = (uint32_t)total - (uint32_t)ihl;
    if (icmp[0] != HAEUN_ICMP_ECHO_REQ || icmp_len > HAEUN_NET_FRAME_MAX)
        return HAEUN_NET_DROP;
    memcpy(icmp_tmp, icmp, icmp_len);
    icmp_tmp[2] = 0u;
    icmp_tmp[3] = 0u;
    icmp_check = haeun_net_checksum(icmp_tmp, icmp_len);
    if (icmp_check !=
        (uint16_t)(((uint16_t)icmp[2] << 8) | (uint16_t)icmp[3])) {
        log_fail("ICMP checksum mismatch");
        return HAEUN_NET_DROP;
    }
    if (out_cap < HAEUN_ETH_HDR_LEN + (uint32_t)total) {
        log_fail("icmp reply buffer too small");
        return HAEUN_NET_NOSPACE;
    }
    haeun_net_eth_write_hdr(out, frame + 6, nif->mac, HAEUN_ETH_TYPE_IPV4);
    memcpy(out + HAEUN_ETH_HDR_LEN, ip, total);
    oip = out + HAEUN_ETH_HDR_LEN;
    oicmp = oip + ihl;
    src = read_u32(ip + 12);
    write_u32(oip + 12, nif->ipv4_be);
    write_u32(oip + 16, src);
    oip[8] = 64u;
    oip[10] = 0u;
    oip[11] = 0u;
    write_u16(oip + 10, haeun_net_checksum(oip, ihl));
    oicmp[0] = HAEUN_ICMP_ECHO_REPLY;
    oicmp[2] = 0u;
    oicmp[3] = 0u;
    write_u16(oicmp + 2, haeun_net_checksum(oicmp, icmp_len));
    *out_len = HAEUN_ETH_HDR_LEN + (uint32_t)total;
    return HAEUN_NET_OK;
}
