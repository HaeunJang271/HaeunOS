#include "haeun/net_backend.h"

#include <stdio.h>
#include <string.h>

#include <ws2tcpip.h>

static void be_log(const char *what) {
    fprintf(stderr, "[haeun-net-backend] %s\n", what != NULL ? what : "");
}

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t rd32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

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

/**
 * Opens a non-blocking host UDP socket and records guest L2/L3 identity.
 */
int haeun_net_backend_init(haeun_net_backend *be, const uint8_t guest_mac[6],
                           const uint8_t peer_mac[6], uint32_t guest_ip) {
    WSADATA wsa;
    u_long nb = 1u;
    if (be == NULL || guest_mac == NULL || peer_mac == NULL)
        return HAEUN_NET_BE_BAD_ARGUMENT;
    memset(be, 0, sizeof(*be));
    be->sock = HAEUN_BE_INVALID_SOCK;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        be_log("WSAStartup failed");
        return HAEUN_NET_BE_IO;
    }
    be->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (be->sock == HAEUN_BE_INVALID_SOCK) {
        be_log("socket failed");
        WSACleanup();
        return HAEUN_NET_BE_IO;
    }
    if (ioctlsocket(be->sock, FIONBIO, &nb) != 0) {
        be_log("FIONBIO failed");
        closesocket(be->sock);
        be->sock = HAEUN_BE_INVALID_SOCK;
        WSACleanup();
        return HAEUN_NET_BE_IO;
    }
    memcpy(be->guest_mac, guest_mac, 6u);
    memcpy(be->peer_mac, peer_mac, 6u);
    be->guest_ip = guest_ip;
    be->ready = 1;
    return HAEUN_NET_BE_OK;
}

/** Closes the host socket and clears state (idempotent). */
void haeun_net_backend_shutdown(haeun_net_backend *be) {
    if (be == NULL)
        return;
    if (be->sock != HAEUN_BE_INVALID_SOCK) {
        closesocket(be->sock);
        be->sock = HAEUN_BE_INVALID_SOCK;
    }
    if (be->ready)
        WSACleanup();
    be->ready = 0;
}

/**
 * Parses a guest TX Ethernet frame; forwards IPv4/UDP via the host socket.
 */
int haeun_net_backend_on_tx(haeun_net_backend *be, const uint8_t *frame,
                            uint32_t length) {
    const uint8_t *ip;
    const uint8_t *udp;
    uint8_t ihl;
    uint16_t total;
    uint16_t udp_len;
    uint32_t dst_ip;
    uint16_t dst_port;
    uint16_t src_port;
    uint32_t plen;
    struct sockaddr_in sa;
    int n;
    if (be == NULL || !be->ready || frame == NULL) {
        be_log("on_tx bad argument");
        return HAEUN_NET_BE_BAD_ARGUMENT;
    }
    if (length < HAEUN_ETH_HDR_LEN + HAEUN_IPV4_HDR_LEN + HAEUN_UDP_HDR_LEN ||
        length > HAEUN_NET_FRAME_MAX) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    if (haeun_net_eth_type(frame, length) != HAEUN_ETH_TYPE_IPV4) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    ip = frame + HAEUN_ETH_HDR_LEN;
    ihl = (uint8_t)((ip[0] & 0x0fu) * 4u);
    if ((ip[0] >> 4) != 4u || ihl != HAEUN_IPV4_HDR_LEN ||
        ip[9] != HAEUN_IP_PROTO_UDP) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    total = rd16(ip + 2);
    if ((uint32_t)HAEUN_ETH_HDR_LEN + (uint32_t)total > length ||
        total < ihl + HAEUN_UDP_HDR_LEN) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    udp = ip + ihl;
    udp_len = rd16(udp + 4);
    if (udp_len < HAEUN_UDP_HDR_LEN ||
        (uint32_t)ihl + (uint32_t)udp_len > (uint32_t)total) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    plen = (uint32_t)udp_len - HAEUN_UDP_HDR_LEN;
    if (plen > HAEUN_NET_UDP_MAX) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    dst_ip = rd32(ip + 16);
    src_port = rd16(udp);
    dst_port = rd16(udp + 2);
    if (dst_ip == 0u || dst_port == 0u || src_port == 0u) {
        be->drops++;
        return HAEUN_NET_BE_DROP;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(dst_port);
    sa.sin_addr.s_addr = htonl(dst_ip);
    n = sendto(be->sock, (const char *)(udp + 8), (int)plen, 0,
               (struct sockaddr *)&sa, (int)sizeof(sa));
    if (n < 0 || (uint32_t)n != plen) {
        be->io_errors++;
        be_log("sendto failed");
        return HAEUN_NET_BE_IO;
    }
    be->last_guest_ip = rd32(ip + 12);
    be->last_guest_port = src_port;
    be->tx_ok++;
    return HAEUN_NET_BE_OK;
}

/**
 * Non-blocking host recv; builds a guest-bound Ethernet/IPv4/UDP frame.
 */
int haeun_net_backend_poll_rx(haeun_net_backend *be, uint8_t *out,
                              uint32_t out_cap, uint32_t *out_len) {
    uint8_t payload[HAEUN_NET_UDP_MAX];
    struct sockaddr_in sa;
    int salen = (int)sizeof(sa);
    int n;
    uint8_t *ip;
    uint8_t *udp;
    uint16_t ulen;
    uint16_t total;
    uint32_t sip;
    uint16_t sport;
    uint32_t dip;
    uint16_t dport;
    if (be == NULL || !be->ready || out == NULL || out_len == NULL)
        return HAEUN_NET_BE_BAD_ARGUMENT;
    *out_len = 0u;
    if (be->last_guest_port == 0u)
        return HAEUN_NET_BE_EMPTY;
    n = recvfrom(be->sock, (char *)payload, (int)sizeof(payload), 0,
                 (struct sockaddr *)&sa, &salen);
    if (n == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
            return HAEUN_NET_BE_EMPTY;
        be->io_errors++;
        return HAEUN_NET_BE_IO;
    }
    if (n < 0 || (uint32_t)n > HAEUN_NET_UDP_MAX)
        return HAEUN_NET_BE_DROP;
    sip = ntohl(sa.sin_addr.s_addr);
    sport = ntohs(sa.sin_port);
    dip = be->last_guest_ip != 0u ? be->last_guest_ip : be->guest_ip;
    dport = be->last_guest_port;
    ulen = (uint16_t)(HAEUN_UDP_HDR_LEN + (uint32_t)n);
    total = (uint16_t)(HAEUN_IPV4_HDR_LEN + ulen);
    if (out_cap < HAEUN_ETH_HDR_LEN + (uint32_t)total)
        return HAEUN_NET_BE_DROP;
    memset(out, 0, HAEUN_ETH_HDR_LEN + total);
    haeun_net_eth_write_hdr(out, be->guest_mac, be->peer_mac,
                            HAEUN_ETH_TYPE_IPV4);
    ip = out + HAEUN_ETH_HDR_LEN;
    ip[0] = 0x45u;
    wr16(ip + 2, total);
    ip[8] = 64u;
    ip[9] = HAEUN_IP_PROTO_UDP;
    wr32(ip + 12, sip);
    wr32(ip + 16, dip);
    wr16(ip + 10, haeun_net_checksum(ip, HAEUN_IPV4_HDR_LEN));
    udp = ip + HAEUN_IPV4_HDR_LEN;
    wr16(udp, sport);
    wr16(udp + 2, dport);
    wr16(udp + 4, ulen);
    /* UDP checksum 0 is valid for IPv4 and accepted by guest udp_input. */
    if (n > 0)
        memcpy(udp + 8, payload, (size_t)n);
    *out_len = HAEUN_ETH_HDR_LEN + total;
    be->rx_ok++;
    return HAEUN_NET_BE_OK;
}
