#include "haeun/net.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-arp] security/validation failure: %s\n", what);
}

static int mac_eq(const uint8_t *a, const uint8_t *b) {
    return memcmp(a, b, 6u) == 0;
}

/**
 * Builds an ARP reply for a request targeting nif->ipv4_be into out.
 */
haeun_net_result haeun_net_arp_reply(const haeun_net_if *nif,
                                     const uint8_t *frame, uint32_t length,
                                     uint8_t *out, uint32_t out_cap,
                                     uint32_t *out_len) {
    const uint8_t *arp;
    uint16_t oper;
    uint32_t tip;
    uint32_t total;
    if (nif == NULL || frame == NULL || out == NULL || out_len == NULL) {
        log_fail("arp_reply bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    *out_len = 0u;
    if (haeun_net_eth_type(frame, length) != HAEUN_ETH_TYPE_ARP)
        return HAEUN_NET_DROP;
    if (length < HAEUN_ETH_HDR_LEN + HAEUN_ARP_HDR_LEN) return HAEUN_NET_DROP;
    arp = frame + HAEUN_ETH_HDR_LEN;
    /* HTYPE=1, PTYPE=0x0800, HLEN=6, PLEN=4 */
    if (arp[0] != 0u || arp[1] != 1u || arp[2] != 0x08u || arp[3] != 0x00u ||
        arp[4] != 6u || arp[5] != 4u)
        return HAEUN_NET_DROP;
    oper = (uint16_t)(((uint16_t)arp[6] << 8) | (uint16_t)arp[7]);
    if (oper != 1u) return HAEUN_NET_DROP; /* request */
    tip = (uint32_t)arp[24] << 24 | (uint32_t)arp[25] << 16 |
          (uint32_t)arp[26] << 8 | (uint32_t)arp[27];
    /* tip is big-endian in the packet; compare as BE uint32 assembled above */
    if (tip != nif->ipv4_be) return HAEUN_NET_DROP;
    total = HAEUN_ETH_HDR_LEN + HAEUN_ARP_HDR_LEN;
    if (out_cap < total) {
        log_fail("arp_reply buffer too small");
        return HAEUN_NET_NOSPACE;
    }
    haeun_net_eth_write_hdr(out, arp + 8, nif->mac, HAEUN_ETH_TYPE_ARP);
    out[14] = 0u;
    out[15] = 1u;
    out[16] = 0x08u;
    out[17] = 0x00u;
    out[18] = 6u;
    out[19] = 4u;
    out[20] = 0u;
    out[21] = 2u; /* reply */
    memcpy(out + 22, nif->mac, 6u);
    out[28] = (uint8_t)(nif->ipv4_be >> 24);
    out[29] = (uint8_t)(nif->ipv4_be >> 16);
    out[30] = (uint8_t)(nif->ipv4_be >> 8);
    out[31] = (uint8_t)nif->ipv4_be;
    memcpy(out + 32, arp + 8, 6u);  /* target MAC = requester SHA */
    memcpy(out + 38, arp + 14, 4u); /* target IP = requester SPA */
    (void)mac_eq;
    *out_len = total;
    return HAEUN_NET_OK;
}
