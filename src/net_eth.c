#include "haeun/net.h"

#include <string.h>

/**
 * Writes a 14-byte Ethernet II header.
 */
void haeun_net_eth_write_hdr(uint8_t *dst, const uint8_t dst_mac[6],
                             const uint8_t src_mac[6], uint16_t ethertype) {
    if (dst == NULL || dst_mac == NULL || src_mac == NULL) return;
    memcpy(dst, dst_mac, 6u);
    memcpy(dst + 6u, src_mac, 6u);
    dst[12] = (uint8_t)(ethertype >> 8);
    dst[13] = (uint8_t)ethertype;
}

/**
 * Returns the Ethernet type field in host order, or 0 if the frame is short.
 */
uint16_t haeun_net_eth_type(const uint8_t *frame, uint32_t length) {
    if (frame == NULL || length < HAEUN_ETH_HDR_LEN) return 0u;
    return (uint16_t)(((uint16_t)frame[12] << 8) | (uint16_t)frame[13]);
}
