#ifndef HAEUN_NET_H
#define HAEUN_NET_H

#include "haeun/vm.h"

#include <stdint.h>

#define HAEUN_ETH_TYPE_ARP 0x0806u
#define HAEUN_ETH_TYPE_IPV4 0x0800u
#define HAEUN_ETH_HDR_LEN 14u
#define HAEUN_ARP_HDR_LEN 28u
#define HAEUN_IPV4_HDR_LEN 20u
#define HAEUN_IP_PROTO_ICMP 1u
#define HAEUN_IP_PROTO_UDP 17u
#define HAEUN_IP_PROTO_TCP 6u
#define HAEUN_ICMP_ECHO_REQ 8u
#define HAEUN_ICMP_ECHO_REPLY 0u
#define HAEUN_UDP_HDR_LEN 8u
#define HAEUN_TCP_HDR_LEN 20u
#define HAEUN_TCP_SYN 0x02u
#define HAEUN_TCP_ACK 0x10u
#define HAEUN_NET_UDP_MAX 512u
#define HAEUN_NET_DNS_MAX_RECORDS 4
#define HAEUN_NET_DNS_NAME_MAX 64

typedef enum {
    HAEUN_NET_OK = 0,
    HAEUN_NET_BAD_ARGUMENT,
    HAEUN_NET_DROP,
    HAEUN_NET_NOSPACE
} haeun_net_result;

typedef enum {
    HAEUN_TCP_CLOSED = 0,
    HAEUN_TCP_LISTEN,
    HAEUN_TCP_SYN_RCVD,
    HAEUN_TCP_ESTABLISHED
} haeun_tcp_state;

typedef struct {
    haeun_vm *vm;
    uint8_t mac[6];
    uint32_t ipv4_be; /* a.b.c.d as (a<<24)|(b<<16)|(c<<8)|d */
    uint32_t tx_pa;
    uint32_t rx_pa;
    uint32_t rx_cap;
    int bound;
    uint16_t udp_port; /* 0 = unbound */
    int udp_pending;
    uint32_t udp_src_ip;
    uint16_t udp_src_port;
    uint8_t udp_src_mac[6];
    uint32_t udp_payload_len;
    uint8_t udp_payload[HAEUN_NET_UDP_MAX];
    uint16_t tcp_port; /* 0 = not listening */
    haeun_tcp_state tcp_state;
    uint8_t tcp_peer_mac[6];
    uint32_t tcp_peer_ip;
    uint16_t tcp_peer_port;
    uint32_t tcp_iss;
    uint32_t tcp_snd_nxt;
    uint32_t tcp_rcv_nxt;
    int dns_enabled;
    int dns_count;
    char dns_names[HAEUN_NET_DNS_MAX_RECORDS][HAEUN_NET_DNS_NAME_MAX];
    uint32_t dns_addrs[HAEUN_NET_DNS_MAX_RECORDS];
} haeun_net_if;

/**
 * Binds a stack interface to a VM with local MAC and IPv4 (host byte order).
 */
haeun_net_result haeun_net_if_init(haeun_net_if *nif, haeun_vm *vm,
                                   const uint8_t mac[6], uint32_t ipv4_host);

/**
 * Programs NIC MAC/DMA and enables the adapter for the interface buffers.
 */
haeun_net_result haeun_net_if_bind(haeun_net_if *nif, uint32_t tx_pa,
                                   uint32_t rx_pa, uint32_t rx_cap);

/**
 * If RX is pending, runs ARP/ICMP/UDP/TCP handling from the guest RX buffer.
 */
haeun_net_result haeun_net_poll(haeun_net_if *nif);

/** Transmits a prebuilt Ethernet frame through the bound NIC TX DMA. */
haeun_net_result haeun_net_tx_frame(haeun_net_if *nif, const uint8_t *frame,
                                    uint32_t length);

/** Internet checksum over data (ones' complement); returns host-order value. */
uint16_t haeun_net_checksum(const uint8_t *data, uint32_t length);

/** Writes an Ethernet II header into dst (14 bytes). */
void haeun_net_eth_write_hdr(uint8_t *dst, const uint8_t dst_mac[6],
                             const uint8_t src_mac[6], uint16_t ethertype);

/** Reads ethertype (host order) from an Ethernet II header. */
uint16_t haeun_net_eth_type(const uint8_t *frame, uint32_t length);

/**
 * If frame is an ARP request for nif->ipv4, builds a reply into out and
 * returns HAEUN_NET_OK with *out_len set; otherwise HAEUN_NET_DROP.
 */
haeun_net_result haeun_net_arp_reply(const haeun_net_if *nif,
                                     const uint8_t *frame, uint32_t length,
                                     uint8_t *out, uint32_t out_cap,
                                     uint32_t *out_len);

/**
 * If frame is an ICMP echo request to nif->ipv4, builds an echo reply into out.
 */
haeun_net_result haeun_net_icmp_echo_reply(const haeun_net_if *nif,
                                           const uint8_t *frame,
                                           uint32_t length, uint8_t *out,
                                           uint32_t out_cap, uint32_t *out_len);

/** Binds the interface to a local UDP port (non-zero). */
haeun_net_result haeun_net_udp_bind(haeun_net_if *nif, uint16_t port);

/**
 * Sends a UDP datagram; dst_mac is the next-hop Ethernet address.
 */
haeun_net_result haeun_net_udp_send(haeun_net_if *nif, const uint8_t dst_mac[6],
                                    uint32_t dst_ip, uint16_t dst_port,
                                    uint16_t src_port, const uint8_t *data,
                                    uint32_t length);

/**
 * If a UDP datagram is pending, copies payload and peer into the outputs.
 */
haeun_net_result haeun_net_udp_recv(haeun_net_if *nif, uint32_t *src_ip,
                                    uint16_t *src_port, uint8_t *out,
                                    uint32_t out_cap, uint32_t *out_len);

/**
 * If frame is UDP to nif->udp_port, copies payload into nif's pending slot.
 */
haeun_net_result haeun_net_udp_input(haeun_net_if *nif, const uint8_t *frame,
                                     uint32_t length);

/** Puts the interface into TCP LISTEN on port. */
haeun_net_result haeun_net_tcp_listen(haeun_net_if *nif, uint16_t port);

/** Returns the current TCP connection state. */
haeun_tcp_state haeun_net_tcp_state(const haeun_net_if *nif);

/**
 * Handles inbound TCP for the listener: SYN→SYN-ACK (fills out), ACK→ESTABLISHED.
 */
haeun_net_result haeun_net_tcp_input(haeun_net_if *nif, const uint8_t *frame,
                                     uint32_t length, uint8_t *out,
                                     uint32_t out_cap, uint32_t *out_len);

/** Registers an authoritative A record (name is ASCII, e.g. "host.haeun"). */
haeun_net_result haeun_net_dns_add_a(haeun_net_if *nif, const char *name,
                                     uint32_t ipv4);

/** Binds UDP on port and enables DNS serving for pending datagrams. */
haeun_net_result haeun_net_dns_enable(haeun_net_if *nif, uint16_t port);

/**
 * If a DNS query is pending on the DNS UDP port, parses it and TX a response.
 */
haeun_net_result haeun_net_dns_serve(haeun_net_if *nif);

#endif




