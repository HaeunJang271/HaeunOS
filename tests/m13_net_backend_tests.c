#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "haeun/net.h"
#include "haeun/net_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#endif

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t build_udp_frame(uint8_t *f, const uint8_t *smac,
                                const uint8_t *dmac, uint32_t sip, uint32_t dip,
                                uint16_t sport, uint16_t dport,
                                const uint8_t *payload, uint32_t plen) {
    uint8_t *ip;
    uint8_t *udp;
    uint16_t ulen = (uint16_t)(8u + plen);
    uint16_t total = (uint16_t)(20u + ulen);
    haeun_net_eth_write_hdr(f, dmac, smac, HAEUN_ETH_TYPE_IPV4);
    ip = f + HAEUN_ETH_HDR_LEN;
    memset(ip, 0, total);
    ip[0] = 0x45u;
    wr16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_UDP;
    wr32(ip + 12, sip);
    wr32(ip + 16, dip);
    wr16(ip + 10, haeun_net_checksum(ip, 20u));
    udp = ip + 20;
    wr16(udp, sport);
    wr16(udp + 2, dport);
    wr16(udp + 4, ulen);
    if (plen != 0u)
        memcpy(udp + 8, payload, plen);
    return HAEUN_ETH_HDR_LEN + total;
}

static int echo_once(haeun_be_sock echo, haeun_net_backend *be) {
    uint8_t buf[64];
    struct sockaddr_in peer;
#ifdef _WIN32
    int plen = (int)sizeof(peer);
    int n = recvfrom(echo, (char *)buf, (int)sizeof(buf), 0,
                     (struct sockaddr *)&peer, &plen);
#else
    socklen_t plen = sizeof(peer);
    ssize_t n = recvfrom(echo, buf, sizeof(buf), 0, (struct sockaddr *)&peer,
                         &plen);
#endif
    if (n <= 0)
        return 0;
#ifdef _WIN32
    (void)sendto(echo, (const char *)buf, n, 0, (struct sockaddr *)&peer, plen);
#else
    (void)sendto(echo, buf, (size_t)n, 0, (struct sockaddr *)&peer, plen);
#endif
    (void)be;
    return 1;
}

static int test_init_shutdown(void) {
    haeun_net_backend be;
    uint8_t gmac[6] = {0x02, 0, 0, 0, 0, 1};
    uint8_t pmac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    CHECK(haeun_net_backend_init(&be, gmac, pmac, 0x0a000002u) ==
          HAEUN_NET_BE_OK);
    CHECK(be.ready == 1);
    haeun_net_backend_shutdown(&be);
    CHECK(be.ready == 0);
    haeun_net_backend_shutdown(&be);
    return 0;
}

static int test_malformed_and_empty(void) {
    haeun_net_backend be;
    uint8_t gmac[6] = {0x02, 0, 0, 0, 0, 1};
    uint8_t pmac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t junk[8] = {0};
    uint8_t out[HAEUN_NET_FRAME_MAX];
    uint32_t olen = 0u;
    CHECK(haeun_net_backend_init(&be, gmac, pmac, 0x0a000002u) ==
          HAEUN_NET_BE_OK);
    CHECK(haeun_net_backend_on_tx(&be, junk, 8u) == HAEUN_NET_BE_DROP);
    CHECK(be.drops >= 1u);
    CHECK(haeun_net_backend_poll_rx(&be, out, sizeof(out), &olen) ==
          HAEUN_NET_BE_EMPTY);
    haeun_net_backend_shutdown(&be);
    return 0;
}

static int test_udp_echo_local(void) {
    haeun_net_backend be;
    uint8_t gmac[6] = {0x02, 0, 0, 0, 0, 1};
    uint8_t pmac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[HAEUN_NET_FRAME_MAX];
    uint8_t out[HAEUN_NET_FRAME_MAX];
    uint8_t payload[2] = {'H', 'I'};
    uint32_t flen;
    uint32_t olen = 0u;
    uint16_t echo_port;
    haeun_be_sock echo;
    struct sockaddr_in addr;
#ifdef _WIN32
    WSADATA wsa;
    CHECK(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
#endif
    echo = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    CHECK(echo != HAEUN_BE_INVALID_SOCK);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001u);
    addr.sin_port = htons(0);
    CHECK(bind(echo, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    {
#ifdef _WIN32
        int alen = (int)sizeof(addr);
#else
        socklen_t alen = sizeof(addr);
#endif
        CHECK(getsockname(echo, (struct sockaddr *)&addr, &alen) == 0);
        echo_port = ntohs(addr.sin_port);
    }
    CHECK(haeun_net_backend_init(&be, gmac, pmac, 0x0a000002u) ==
          HAEUN_NET_BE_OK);
    flen = build_udp_frame(frame, gmac, pmac, 0x0a000002u, 0x7f000001u, 7u,
                           echo_port, payload, 2u);
    CHECK(haeun_net_backend_on_tx(&be, frame, flen) == HAEUN_NET_BE_OK);
    CHECK(be.tx_ok == 1u);
    CHECK(echo_once(echo, &be) == 1);
    CHECK(haeun_net_backend_poll_rx(&be, out, sizeof(out), &olen) ==
          HAEUN_NET_BE_OK);
    CHECK(olen >= HAEUN_ETH_HDR_LEN + 28u);
    CHECK(out[HAEUN_ETH_HDR_LEN + 28u] == (uint8_t)'H');
    CHECK(out[HAEUN_ETH_HDR_LEN + 29u] == (uint8_t)'I');
    CHECK(be.rx_ok == 1u);
    haeun_net_backend_shutdown(&be);
#ifdef _WIN32
    closesocket(echo);
    WSACleanup();
#else
    close(echo);
#endif
    return 0;
}

static int test_dns_external_optional(void) {
    const char *ext = getenv("HAEUN_NET_EXTERNAL");
    haeun_net_backend be;
    uint8_t gmac[6] = {0x02, 0, 0, 0, 0, 1};
    uint8_t pmac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t frame[HAEUN_NET_FRAME_MAX];
    uint8_t out[HAEUN_NET_FRAME_MAX];
    /* Minimal DNS query: id=0x1234, QD=example.com A IN */
    uint8_t dns[] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x07, 'e',  'x',  'a',  'm',  'p',  'l',  'e',
                     0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01};
    uint32_t flen;
    uint32_t olen = 0u;
    int i;
    int got = 0;
    if (ext == NULL || ext[0] == '\0' || strcmp(ext, "0") == 0) {
        puts("dns_external: skipped (set HAEUN_NET_EXTERNAL=1)");
        return 0;
    }
    CHECK(haeun_net_backend_init(&be, gmac, pmac, 0x0a000002u) ==
          HAEUN_NET_BE_OK);
    flen = build_udp_frame(frame, gmac, pmac, 0x0a000002u, 0x08080808u, 40000u,
                           53u, dns, (uint32_t)sizeof(dns));
    CHECK(haeun_net_backend_on_tx(&be, frame, flen) == HAEUN_NET_BE_OK);
    for (i = 0; i < 200; ++i) {
#ifdef _WIN32
        Sleep(10);
#else
        {
            struct timespec req = {.tv_sec = 0, .tv_nsec = 10000000L}; /* 10ms */
            struct timespec rem;
            while (nanosleep(&req, &rem) != 0) {
                if (errno != EINTR)
                    break;
                req = rem;
            }
        }
#endif
        if (haeun_net_backend_poll_rx(&be, out, sizeof(out), &olen) ==
            HAEUN_NET_BE_OK) {
            got = 1;
            break;
        }
    }
    CHECK(got == 1);
    CHECK(olen > HAEUN_ETH_HDR_LEN + 28u);
    puts("dns_external: ok");
    haeun_net_backend_shutdown(&be);
    return 0;
}

int main(void) {
    if (test_init_shutdown() != 0)
        return 1;
    if (test_malformed_and_empty() != 0)
        return 1;
    if (test_udp_echo_local() != 0)
        return 1;
    if (test_dns_external_optional() != 0)
        return 1;
    puts("m13_net_backend_tests ok");
    return 0;
}
