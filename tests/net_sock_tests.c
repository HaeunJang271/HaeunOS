#include "haeun/net.h"
#include "haeun/net_sock.h"
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

static uint32_t build_tcp(uint8_t *f, const uint8_t *smac, const uint8_t *dmac,
                          uint32_t sip, uint32_t dip, uint16_t sport,
                          uint16_t dport, uint32_t seq, uint32_t ack,
                          uint8_t flags) {
    uint8_t *ip;
    uint8_t *tcp;
    uint16_t total = 40u;
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

static int test_udp_socket_path(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    tx_cap cap;
    int fd = -1;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t msg[] = {'o', 'k'};
    uint8_t frame[128];
    uint8_t out[8];
    uint32_t out_len = 0u, src_ip = 0u, flen;
    uint16_t src_port = 0u;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_DGRAM, &fd) == HAEUN_NET_OK);
    CHECK(haeun_sock_bind(&ss, fd, 7u) == HAEUN_NET_OK);
    CHECK(haeun_sock_sendto(&ss, fd, peer, 0x0a000001u, 9u, msg, 2u) ==
          HAEUN_NET_OK);
    CHECK(cap.count == 1);
    flen = build_udp(frame, peer, mac, 0x0a000001u, 0x0a000002u, 5000u, 7u, msg,
                     2u);
    CHECK(haeun_vm_net_inject(&vm, frame, flen));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(haeun_sock_recvfrom(&ss, fd, &src_ip, &src_port, out, sizeof(out),
                              &out_len) == HAEUN_NET_OK);
    CHECK(out_len == 2u && out[0] == 'o' && out[1] == 'k');
    CHECK(src_ip == 0x0a000001u && src_port == 5000u);
    CHECK(haeun_sock_close(&ss, fd) == HAEUN_NET_OK);
    return 0;
}

static int test_tcp_accept_path(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    tx_cap cap;
    int lfd = -1, cfd = -1;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t syn[64], ack[64];
    uint32_t syn_len, ack_len, peer_ip = 0u, iss;
    uint16_t peer_port = 0u;
    const uint8_t *otcp;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, HAEUN_SOCK_STREAM, &lfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_bind(&ss, lfd, 80u) == HAEUN_NET_OK);
    CHECK(haeun_sock_listen(&ss, lfd) == HAEUN_NET_OK);
    syn_len = build_tcp(syn, peer, mac, 0x0a000001u, 0x0a000002u, 40000u, 80u,
                        5000u, 0u, HAEUN_TCP_SYN);
    CHECK(haeun_vm_net_inject(&vm, syn, syn_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    otcp = cap.buf + HAEUN_ETH_HDR_LEN + 20;
    iss = read_u32(otcp + 4);
    ack_len = build_tcp(ack, peer, mac, 0x0a000001u, 0x0a000002u, 40000u, 80u,
                        5001u, iss + 1u, HAEUN_TCP_ACK);
    CHECK(haeun_vm_net_inject(&vm, ack, ack_len));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(haeun_sock_accept(&ss, lfd, &cfd, &peer_ip, &peer_port) ==
          HAEUN_NET_OK);
    CHECK(cfd >= 0 && peer_ip == 0x0a000001u && peer_port == 40000u);
    CHECK(haeun_sock_close(&ss, cfd) == HAEUN_NET_OK);
    CHECK(haeun_sock_close(&ss, lfd) == HAEUN_NET_OK);
    return 0;
}

static int test_sock_validation(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    int fd = -1;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    CHECK(haeun_socket(&ss, 99, &fd) == HAEUN_NET_BAD_ARGUMENT);
    CHECK(haeun_sock_close(&ss, 0) == HAEUN_NET_BAD_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_udp_socket_path() != 0) return 1;
    if (test_tcp_accept_path() != 0) return 1;
    if (test_sock_validation() != 0) return 1;
    puts("all net sock tests passed");
    return 0;
}
