#include "haeun/net.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum { TX_PA = 0x2000u, RX_PA = 0x3000u, RX_CAP = 512u };

typedef struct {
    uint8_t buf[HAEUN_NET_FRAME_MAX];
    uint32_t len;
    int count;
} tx_cap;

static void on_tx(void *ctx, const uint8_t *frame, uint32_t length) {
    tx_cap *c = (tx_cap *)ctx;
    if (c == NULL || frame == NULL || length > HAEUN_NET_FRAME_MAX) return;
    memcpy(c->buf, frame, length);
    c->len = length;
    c->count++;
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

/**
 * Builds a minimal IPv4 ICMP echo request Ethernet frame.
 */
static uint32_t build_echo_req(uint8_t *f, const uint8_t *src_mac,
                               const uint8_t *dst_mac, uint32_t src_ip,
                               uint32_t dst_ip, uint16_t id, uint16_t seq,
                               const uint8_t *payload, uint32_t plen) {
    uint8_t *ip;
    uint8_t *icmp;
    uint16_t icmp_len = (uint16_t)(8u + plen);
    uint16_t total = (uint16_t)(HAEUN_IPV4_HDR_LEN + icmp_len);
    haeun_net_eth_write_hdr(f, dst_mac, src_mac, HAEUN_ETH_TYPE_IPV4);
    ip = f + HAEUN_ETH_HDR_LEN;
    memset(ip, 0, total);
    ip[0] = 0x45u;
    write_u16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_ICMP;
    write_u32(ip + 12, src_ip);
    write_u32(ip + 16, dst_ip);
    write_u16(ip + 10, haeun_net_checksum(ip, HAEUN_IPV4_HDR_LEN));
    icmp = ip + HAEUN_IPV4_HDR_LEN;
    icmp[0] = HAEUN_ICMP_ECHO_REQ;
    write_u16(icmp + 4, id);
    write_u16(icmp + 6, seq);
    if (plen != 0u) memcpy(icmp + 8, payload, plen);
    write_u16(icmp + 2, haeun_net_checksum(icmp, icmp_len));
    return HAEUN_ETH_HDR_LEN + total;
}

/**
 * Echo request through NIC yields a real echo reply on the TX sink.
 */
static int test_icmp_echo_reply_path(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t req[128];
    uint8_t payload[4] = {0xde, 0xad, 0xbe, 0xef};
    uint32_t our_ip = 0x0a000002u;
    uint32_t peer_ip = 0x0a000001u;
    uint32_t req_len;
    const uint8_t *oip;
    const uint8_t *oicmp;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    req_len = build_echo_req(req, peer, mac, peer_ip, our_ip, 0x1234u, 0x0007u,
                             payload, 4u);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, our_ip) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, req, req_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    CHECK(cap.len == req_len);
    CHECK(haeun_net_eth_type(cap.buf, cap.len) == HAEUN_ETH_TYPE_IPV4);
    CHECK(memcmp(cap.buf, peer, 6u) == 0);
    CHECK(memcmp(cap.buf + 6, mac, 6u) == 0);
    oip = cap.buf + HAEUN_ETH_HDR_LEN;
    CHECK(oip[9] == HAEUN_IP_PROTO_ICMP);
    CHECK(oip[12] == 0x0au && oip[13] == 0x00u && oip[14] == 0x00u &&
          oip[15] == 0x02u);
    CHECK(oip[16] == 0x0au && oip[17] == 0x00u && oip[18] == 0x00u &&
          oip[19] == 0x01u);
    {
        uint8_t hdr[20];
        memcpy(hdr, oip, 20u);
        hdr[10] = 0u;
        hdr[11] = 0u;
        CHECK(haeun_net_checksum(hdr, 20u) ==
              (uint16_t)(((uint16_t)oip[10] << 8) | (uint16_t)oip[11]));
    }
    oicmp = oip + HAEUN_IPV4_HDR_LEN;
    CHECK(oicmp[0] == HAEUN_ICMP_ECHO_REPLY);
    CHECK(oicmp[4] == 0x12u && oicmp[5] == 0x34u);
    CHECK(oicmp[6] == 0x00u && oicmp[7] == 0x07u);
    CHECK(memcmp(oicmp + 8, payload, 4u) == 0);
    {
        uint8_t tmp[16];
        uint32_t ilen = 12u;
        memcpy(tmp, oicmp, ilen);
        tmp[2] = 0u;
        tmp[3] = 0u;
        CHECK(haeun_net_checksum(tmp, ilen) ==
              (uint16_t)(((uint16_t)oicmp[2] << 8) | (uint16_t)oicmp[3]));
    }
    return 0;
}

/**
 * Bad IPv4 checksum is dropped (no TX).
 */
static int test_bad_checksum_dropped(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t req[128];
    uint32_t req_len;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    req_len = build_echo_req(req, peer, mac, 0x0a000001u, 0x0a000002u, 1u, 1u,
                             NULL, 0u);
    req[HAEUN_ETH_HDR_LEN + 10] ^= 0xffu; /* corrupt IP checksum */
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, req, req_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_DROP);
    CHECK(cap.count == 0);
    return 0;
}

int main(void) {
    if (test_icmp_echo_reply_path() != 0) return 1;
    if (test_bad_checksum_dropped() != 0) return 1;
    puts("all net icmp tests passed");
    return 0;
}
