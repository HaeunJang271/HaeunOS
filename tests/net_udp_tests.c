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

static uint16_t udp_csum(uint32_t src, uint32_t dst, const uint8_t *udp,
                         uint32_t ulen) {
    uint8_t pseudo[12];
    uint32_t sum = 0u;
    uint32_t i;
    write_u32(pseudo, src);
    write_u32(pseudo + 4, dst);
    pseudo[8] = 0u;
    pseudo[9] = HAEUN_IP_PROTO_UDP;
    write_u16(pseudo + 10, (uint16_t)ulen);
    for (i = 0u; i < 12u; i += 2u)
        sum += ((uint32_t)pseudo[i] << 8) | (uint32_t)pseudo[i + 1u];
    for (i = 0u; i + 1u < ulen; i += 2u)
        sum += ((uint32_t)udp[i] << 8) | (uint32_t)udp[i + 1u];
    if ((ulen & 1u) != 0u) sum += (uint32_t)udp[ulen - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xffffu) + (sum >> 16);
    sum = ~sum & 0xffffu;
    return sum == 0u ? 0xffffu : (uint16_t)sum;
}

static uint32_t build_udp(uint8_t *f, const uint8_t *smac, const uint8_t *dmac,
                          uint32_t sip, uint32_t dip, uint16_t sport,
                          uint16_t dport, const uint8_t *payload, uint32_t plen) {
    uint8_t *ip;
    uint8_t *udp;
    uint16_t ulen = (uint16_t)(8u + plen);
    uint16_t total = (uint16_t)(20u + ulen);
    haeun_net_eth_write_hdr(f, dmac, smac, HAEUN_ETH_TYPE_IPV4);
    ip = f + HAEUN_ETH_HDR_LEN;
    memset(ip, 0, total);
    ip[0] = 0x45u;
    write_u16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_UDP;
    write_u32(ip + 12, sip);
    write_u32(ip + 16, dip);
    write_u16(ip + 10, haeun_net_checksum(ip, 20u));
    udp = ip + 20;
    write_u16(udp, sport);
    write_u16(udp + 2, dport);
    write_u16(udp + 4, ulen);
    if (plen) memcpy(udp + 8, payload, plen);
    write_u16(udp + 6, udp_csum(sip, dip, udp, ulen));
    return HAEUN_ETH_HDR_LEN + total;
}

/**
 * UDP send produces a real IPv4/UDP frame on the NIC TX sink.
 */
static int test_udp_send(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t msg[] = {'h', 'i'};
    const uint8_t *udp;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_udp_send(&nif, peer, 0x0a000001u, 9u, 4000u, msg, 2u) ==
          HAEUN_NET_OK);
    CHECK(cap.count == 1);
    CHECK(haeun_net_eth_type(cap.buf, cap.len) == HAEUN_ETH_TYPE_IPV4);
    CHECK(cap.buf[HAEUN_ETH_HDR_LEN + 9] == HAEUN_IP_PROTO_UDP);
    udp = cap.buf + HAEUN_ETH_HDR_LEN + 20;
    CHECK(udp[0] == 0x0fu && udp[1] == 0xa0u); /* 4000 */
    CHECK(udp[2] == 0u && udp[3] == 9u);
    CHECK(memcmp(udp + 8, msg, 2u) == 0);
    return 0;
}

/**
 * Injected UDP to the bound port is delivered via poll/recv.
 */
static int test_udp_recv_path(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[128];
    uint8_t payload[] = {1, 2, 3, 4, 5};
    uint8_t out[16];
    uint32_t out_len = 0u;
    uint32_t src_ip = 0u;
    uint16_t src_port = 0u;
    uint32_t flen;
    memset(ram, 0, sizeof(ram));
    flen = build_udp(frame, peer, mac, 0x0a000001u, 0x0a000002u, 5000u, 7u,
                     payload, 5u);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_udp_bind(&nif, 7u) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, frame, flen));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(haeun_net_udp_recv(&nif, &src_ip, &src_port, out, sizeof(out),
                             &out_len) == HAEUN_NET_OK);
    CHECK(out_len == 5u);
    CHECK(memcmp(out, payload, 5u) == 0);
    CHECK(src_ip == 0x0a000001u);
    CHECK(src_port == 5000u);
    return 0;
}

/**
 * UDP to a different port is dropped.
 */
static int test_udp_port_mismatch(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[128];
    uint32_t flen;
    uint8_t out[8];
    uint32_t out_len = 0u;
    uint32_t src_ip = 0u;
    uint16_t src_port = 0u;
    memset(ram, 0, sizeof(ram));
    flen = build_udp(frame, peer, mac, 0x0a000001u, 0x0a000002u, 1u, 99u, NULL,
                     0u);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_udp_bind(&nif, 7u) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, frame, flen));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_DROP);
    CHECK(haeun_net_udp_recv(&nif, &src_ip, &src_port, out, sizeof(out),
                             &out_len) == HAEUN_NET_DROP);
    return 0;
}

int main(void) {
    if (test_udp_send() != 0) return 1;
    if (test_udp_recv_path() != 0) return 1;
    if (test_udp_port_mismatch() != 0) return 1;
    puts("all net udp tests passed");
    return 0;
}
