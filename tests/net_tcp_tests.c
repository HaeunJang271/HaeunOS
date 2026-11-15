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

static uint32_t read_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t tcp_csum(uint32_t src, uint32_t dst, const uint8_t *tcp,
                         uint32_t tlen) {
    uint8_t pseudo[12];
    uint32_t sum = 0u;
    uint32_t i;
    write_u32(pseudo, src);
    write_u32(pseudo + 4, dst);
    pseudo[8] = 0u;
    pseudo[9] = HAEUN_IP_PROTO_TCP;
    write_u16(pseudo + 10, (uint16_t)tlen);
    for (i = 0u; i < 12u; i += 2u)
        sum += ((uint32_t)pseudo[i] << 8) | (uint32_t)pseudo[i + 1u];
    for (i = 0u; i + 1u < tlen; i += 2u)
        sum += ((uint32_t)tcp[i] << 8) | (uint32_t)tcp[i + 1u];
    if ((tlen & 1u) != 0u) sum += (uint32_t)tcp[tlen - 1u] << 8;
    while ((sum >> 16) != 0u) sum = (sum & 0xffffu) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint32_t build_tcp(uint8_t *f, const uint8_t *smac, const uint8_t *dmac,
                          uint32_t sip, uint32_t dip, uint16_t sport,
                          uint16_t dport, uint32_t seq, uint32_t ack,
                          uint8_t flags) {
    uint8_t *ip;
    uint8_t *tcp;
    uint16_t total = HAEUN_IPV4_HDR_LEN + HAEUN_TCP_HDR_LEN;
    haeun_net_eth_write_hdr(f, dmac, smac, HAEUN_ETH_TYPE_IPV4);
    ip = f + HAEUN_ETH_HDR_LEN;
    memset(ip, 0, total);
    ip[0] = 0x45u;
    write_u16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_TCP;
    write_u32(ip + 12, sip);
    write_u32(ip + 16, dip);
    write_u16(ip + 10, haeun_net_checksum(ip, 20u));
    tcp = ip + 20;
    write_u16(tcp, sport);
    write_u16(tcp + 2, dport);
    write_u32(tcp + 4, seq);
    write_u32(tcp + 8, ack);
    tcp[12] = 0x50u;
    tcp[13] = flags;
    write_u16(tcp + 14, 65535u);
    write_u16(tcp + 16, tcp_csum(sip, dip, tcp, 20u));
    return HAEUN_ETH_HDR_LEN + total;
}

/**
 * SYN then ACK through the NIC completes a passive handshake.
 */
static int test_tcp_handshake(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t syn[64];
    uint8_t ack[64];
    uint32_t syn_len;
    uint32_t ack_len;
    const uint8_t *otcp;
    uint32_t iss;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    syn_len = build_tcp(syn, peer, mac, 0x0a000001u, 0x0a000002u, 40000u, 80u,
                        5000u, 0u, HAEUN_TCP_SYN);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_tcp_listen(&nif, 80u) == HAEUN_NET_OK);
    CHECK(haeun_net_tcp_state(&nif) == HAEUN_TCP_LISTEN);
    CHECK(haeun_vm_net_inject(&vm, syn, syn_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    CHECK(haeun_net_tcp_state(&nif) == HAEUN_TCP_SYN_RCVD);
    otcp = cap.buf + HAEUN_ETH_HDR_LEN + 20;
    CHECK((otcp[13] & HAEUN_TCP_SYN) != 0u);
    CHECK((otcp[13] & HAEUN_TCP_ACK) != 0u);
    CHECK(read_u32(otcp + 8) == 5001u); /* ack peer seq+1 */
    iss = read_u32(otcp + 4);
    ack_len = build_tcp(ack, peer, mac, 0x0a000001u, 0x0a000002u, 40000u, 80u,
                        5001u, iss + 1u, HAEUN_TCP_ACK);
    CHECK(haeun_vm_net_inject(&vm, ack, ack_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(haeun_net_tcp_state(&nif) == HAEUN_TCP_ESTABLISHED);
    CHECK(cap.count == 1); /* no extra TX on final ACK */
    return 0;
}

/**
 * TCP to a non-listening port is dropped.
 */
static int test_tcp_wrong_port(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t syn[64];
    uint32_t syn_len;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    syn_len = build_tcp(syn, peer, mac, 0x0a000001u, 0x0a000002u, 1u, 81u, 1u,
                        0u, HAEUN_TCP_SYN);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_tcp_listen(&nif, 80u) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, syn, syn_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_DROP);
    CHECK(cap.count == 0);
    CHECK(haeun_net_tcp_state(&nif) == HAEUN_TCP_LISTEN);
    return 0;
}

int main(void) {
    if (test_tcp_handshake() != 0) return 1;
    if (test_tcp_wrong_port() != 0) return 1;
    puts("all net tcp tests passed");
    return 0;
}
