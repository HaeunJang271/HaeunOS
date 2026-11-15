#include "haeun/net.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum {
    DNS_HDR = 12u,
    DNS_TYPE_A = 1u,
    DNS_CLASS_IN = 1u,
    DNS_RCODE_NXDOMAIN = 3u
};

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-net-dns] security/validation failure: %s\n", what);
}

static void write_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static void name_lower(char *s) {
    while (*s != '\0') {
        *s = (char)tolower((unsigned char)*s);
        ++s;
    }
}

static int parse_qname(const uint8_t *msg, uint32_t len, uint32_t *off,
                       char *out, size_t out_cap) {
    size_t o = 0u;
    uint32_t p = *off;
    int labels = 0;
    if (out_cap == 0u) return 0;
    out[0] = '\0';
    while (p < len) {
        uint8_t lab = msg[p++];
        if (lab == 0u) {
            *off = p;
            if (o == 0u) {
                out[0] = '.';
                out[1] = '\0';
            }
            name_lower(out);
            return 1;
        }
        if ((lab & 0xc0u) != 0u) {
            log_fail("compressed qname unsupported");
            return 0;
        }
        if (p + lab > len) return 0;
        if (labels > 0) {
            if (o + 1u >= out_cap) return 0;
            out[o++] = '.';
        }
        if (o + lab >= out_cap) return 0;
        memcpy(out + o, msg + p, lab);
        o += lab;
        out[o] = '\0';
        p += lab;
        ++labels;
    }
    return 0;
}

static int lookup_a(const haeun_net_if *nif, const char *name, uint32_t *ip) {
    int i;
    for (i = 0; i < nif->dns_count; ++i) {
        if (strcmp(nif->dns_names[i], name) == 0) {
            *ip = nif->dns_addrs[i];
            return 1;
        }
    }
    return 0;
}

/**
 * Adds an authoritative A record to the local DNS zone.
 */
haeun_net_result haeun_net_dns_add_a(haeun_net_if *nif, const char *name,
                                     uint32_t ipv4) {
    size_t n;
    if (nif == NULL || name == NULL || name[0] == '\0') {
        log_fail("dns_add_a bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    n = strlen(name);
    if (n >= (size_t)HAEUN_NET_DNS_NAME_MAX) {
        log_fail("dns_add_a name too long");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (nif->dns_count >= HAEUN_NET_DNS_MAX_RECORDS) {
        log_fail("dns_add_a zone full");
        return HAEUN_NET_NOSPACE;
    }
    memcpy(nif->dns_names[nif->dns_count], name, n + 1u);
    name_lower(nif->dns_names[nif->dns_count]);
    nif->dns_addrs[nif->dns_count] = ipv4;
    nif->dns_count++;
    return HAEUN_NET_OK;
}

/**
 * Enables DNS serving on the given UDP port (binds the port).
 */
haeun_net_result haeun_net_dns_enable(haeun_net_if *nif, uint16_t port) {
    haeun_net_result r;
    if (nif == NULL || !nif->bound || port == 0u) {
        log_fail("dns_enable bad argument");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    r = haeun_net_udp_bind(nif, port);
    if (r != HAEUN_NET_OK) return r;
    nif->dns_enabled = 1;
    return HAEUN_NET_OK;
}

/**
 * Parses a pending DNS query and transmits a real response via UDP/NIC.
 */
haeun_net_result haeun_net_dns_serve(haeun_net_if *nif) {
    const uint8_t *q;
    uint32_t qlen;
    uint32_t off;
    char qname[HAEUN_NET_DNS_NAME_MAX];
    uint16_t qtype;
    uint16_t qclass;
    uint16_t id;
    uint8_t resp[HAEUN_NET_UDP_MAX];
    uint32_t rlen;
    uint32_t ans_ip = 0u;
    int found;
    uint16_t flags;
    haeun_net_result r;
    if (nif == NULL || !nif->dns_enabled) {
        log_fail("dns_serve not enabled");
        return HAEUN_NET_BAD_ARGUMENT;
    }
    if (!nif->udp_pending) return HAEUN_NET_DROP;
    q = nif->udp_payload;
    qlen = nif->udp_payload_len;
    if (qlen < DNS_HDR + 5u) {
        log_fail("dns_serve short query");
        nif->udp_pending = 0;
        return HAEUN_NET_DROP;
    }
    id = read_u16(q);
    if ((q[2] & 0x80u) != 0u) {
        nif->udp_pending = 0;
        return HAEUN_NET_DROP; /* not a query */
    }
    if (read_u16(q + 4) != 1u) {
        log_fail("dns_serve qdcount != 1");
        nif->udp_pending = 0;
        return HAEUN_NET_DROP;
    }
    off = DNS_HDR;
    if (!parse_qname(q, qlen, &off, qname, sizeof(qname))) {
        log_fail("dns_serve bad qname");
        nif->udp_pending = 0;
        return HAEUN_NET_DROP;
    }
    if (off + 4u > qlen) {
        nif->udp_pending = 0;
        return HAEUN_NET_DROP;
    }
    qtype = read_u16(q + off);
    qclass = read_u16(q + off + 2u);
    off += 4u;
    found = (qtype == DNS_TYPE_A && qclass == DNS_CLASS_IN &&
             lookup_a(nif, qname, &ans_ip));
    /* Build response: copy question, set flags/counts, optional answer. */
    if (off > HAEUN_NET_UDP_MAX) {
        nif->udp_pending = 0;
        return HAEUN_NET_NOSPACE;
    }
    memcpy(resp, q, off);
    flags = (uint16_t)(0x8000u | 0x0400u); /* QR | AA */
    if (!found) flags = (uint16_t)(flags | DNS_RCODE_NXDOMAIN);
    write_u16(resp, id);
    write_u16(resp + 2, flags);
    write_u16(resp + 4, 1u); /* QDCOUNT */
    write_u16(resp + 6, found ? 1u : 0u);
    write_u16(resp + 8, 0u);
    write_u16(resp + 10, 0u);
    rlen = off;
    if (found) {
        if (rlen + 16u > HAEUN_NET_UDP_MAX) {
            nif->udp_pending = 0;
            return HAEUN_NET_NOSPACE;
        }
        resp[rlen++] = 0xc0u;
        resp[rlen++] = 0x0cu; /* pointer to QNAME at offset 12 */
        write_u16(resp + rlen, DNS_TYPE_A);
        rlen += 2u;
        write_u16(resp + rlen, DNS_CLASS_IN);
        rlen += 2u;
        resp[rlen++] = 0u;
        resp[rlen++] = 0u;
        resp[rlen++] = 0u;
        resp[rlen++] = 60u; /* TTL */
        write_u16(resp + rlen, 4u);
        rlen += 2u;
        resp[rlen++] = (uint8_t)(ans_ip >> 24);
        resp[rlen++] = (uint8_t)(ans_ip >> 16);
        resp[rlen++] = (uint8_t)(ans_ip >> 8);
        resp[rlen++] = (uint8_t)ans_ip;
    }
    nif->udp_pending = 0;
    r = haeun_net_udp_send(nif, nif->udp_src_mac, nif->udp_src_ip,
                           nif->udp_src_port, nif->udp_port, resp, rlen);
    return r;
}
