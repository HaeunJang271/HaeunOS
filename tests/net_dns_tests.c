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

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
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

static uint32_t encode_name(uint8_t *dst, const char *name) {
    uint32_t o = 0u;
    while (*name) {
        const char *dot = strchr(name, '.');
        size_t lab = dot ? (size_t)(dot - name) : strlen(name);
        dst[o++] = (uint8_t)lab;
        memcpy(dst + o, name, lab);
        o += (uint32_t)lab;
        if (!dot) break;
        name = dot + 1;
    }
    dst[o++] = 0u;
    return o;
}

static uint32_t build_dns_query_udp(uint8_t *f, const uint8_t *smac,
                                    const uint8_t *dmac, uint32_t sip,
                                    uint32_t dip, uint16_t sport,
                                    const char *qname) {
    uint8_t dns[128];
    uint32_t dlen = 0u;
    uint8_t *ip;
    uint8_t *udp;
    uint16_t ulen;
    uint16_t total;
    write_u16(dns, 0x1234u);
    write_u16(dns + 2, 0x0100u); /* RD */
    write_u16(dns + 4, 1u);
    write_u16(dns + 6, 0u);
    write_u16(dns + 8, 0u);
    write_u16(dns + 10, 0u);
    dlen = 12u;
    dlen += encode_name(dns + dlen, qname);
    write_u16(dns + dlen, 1u); /* A */
    dlen += 2u;
    write_u16(dns + dlen, 1u); /* IN */
    dlen += 2u;
    ulen = (uint16_t)(8u + dlen);
    total = (uint16_t)(20u + ulen);
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
    write_u16(udp + 2, 53u);
    write_u16(udp + 4, ulen);
    memcpy(udp + 8, dns, dlen);
    write_u16(udp + 6, udp_csum(sip, dip, udp, ulen));
    return HAEUN_ETH_HDR_LEN + total;
}

/**
 * Registered A name yields a DNS answer through the NIC TX path.
 */
static int test_dns_a_reply(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[256];
    uint32_t flen;
    const uint8_t *udp;
    const uint8_t *dns;
    uint16_t ulen;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    flen = build_dns_query_udp(frame, peer, mac, 0x0a000001u, 0x0a000002u,
                               53000u, "host.haeun");
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_dns_add_a(&nif, "host.haeun", 0x0a000064u) == HAEUN_NET_OK);
    CHECK(haeun_net_dns_enable(&nif, 53u) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, frame, flen));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    udp = cap.buf + HAEUN_ETH_HDR_LEN + 20;
    ulen = read_u16(udp + 4);
    dns = udp + 8;
    CHECK(read_u16(dns) == 0x1234u);
    CHECK((dns[2] & 0x80u) != 0u); /* QR */
    CHECK(read_u16(dns + 6) == 1u); /* ANCOUNT */
    {
        uint32_t dns_len = (uint32_t)ulen - 8u;
        CHECK(dns_len >= 16u);
        CHECK(dns[dns_len - 4u] == 0x0au);
        CHECK(dns[dns_len - 3u] == 0x00u);
        CHECK(dns[dns_len - 2u] == 0x00u);
        CHECK(dns[dns_len - 1u] == 0x64u);
    }
    return 0;
}

/**
 * Unknown name returns NXDOMAIN (still a real DNS response).
 */
static int test_dns_nxdomain(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[256];
    uint32_t flen;
    const uint8_t *dns;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    flen = build_dns_query_udp(frame, peer, mac, 0x0a000001u, 0x0a000002u,
                               53000u, "missing.haeun");
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_net_dns_enable(&nif, 53u) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, frame, flen));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    dns = cap.buf + HAEUN_ETH_HDR_LEN + 20 + 8;
    CHECK((dns[3] & 0x0fu) == 3u); /* RCODE NXDOMAIN */
    CHECK(read_u16(dns + 6) == 0u);
    return 0;
}

int main(void) {
    if (test_dns_a_reply() != 0) return 1;
    if (test_dns_nxdomain() != 0) return 1;
    puts("all net dns tests passed");
    return 0;
}
