#include "haeun/vm_net_host.h"

#include <stdio.h>
#include <string.h>

static void on_tx(void *ctx, const uint8_t *frame, uint32_t length) {
    haeun_vm_net_host *host = (haeun_vm_net_host *)ctx;
    if (host == NULL || frame == NULL || length > HAEUN_NET_FRAME_MAX)
        return;
    memcpy(host->tx.buf, frame, length);
    host->tx.len = length;
    host->tx.count++;
    if (host->real_mode)
        (void)haeun_net_backend_on_tx(&host->be, frame, length);
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
    if ((ulen & 1u) != 0u)
        sum += (uint32_t)udp[ulen - 1u] << 8;
    while ((sum >> 16) != 0u)
        sum = (sum & 0xffffu) + (sum >> 16);
    sum = ~sum & 0xffffu;
    return sum == 0u ? 0xffffu : (uint16_t)sum;
}

static uint32_t build_udp_reply(uint8_t *f, const uint8_t *smac,
                                const uint8_t *dmac, uint32_t sip, uint32_t dip,
                                uint16_t sport, uint16_t dport,
                                const uint8_t *payload, uint32_t plen) {
    uint8_t *ip;
    uint8_t *udp;
    uint16_t ulen = (uint16_t)(8u + plen);
    uint16_t total = (uint16_t)(20u + ulen);
    uint16_t csum;
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
    if (plen != 0u && payload != NULL)
        memcpy(udp + 8, payload, plen);
    csum = udp_csum(sip, dip, udp, ulen);
    write_u16(udp + 6, csum);
    return HAEUN_ETH_HDR_LEN + total;
}

/**
 * Like attach; real_mode!=0 enables the user-mode UDP host backend.
 */
haeun_net_result haeun_vm_net_host_attach_ex(haeun_vm_net_host *host,
                                             haeun_vm *vm, int real_mode) {
    haeun_net_result r;
    if (host == NULL || vm == NULL)
        return HAEUN_NET_BAD_ARGUMENT;
    memset(host, 0, sizeof(*host));
    host->mac[0] = 0x02u;
    host->mac[5] = 0x01u;
    host->peer_mac[0] = 0xaau;
    host->peer_mac[1] = 0xbbu;
    host->peer_mac[2] = 0xccu;
    host->peer_mac[3] = 0xddu;
    host->peer_mac[4] = 0xeeu;
    host->peer_mac[5] = 0xffu;
    host->real_mode = real_mode != 0 ? 1 : 0;
    haeun_vm_net_set_tx_sink(vm, on_tx, host);
    r = haeun_net_if_init(&host->nif, vm, host->mac, 0x0a000002u);
    if (r != HAEUN_NET_OK)
        return r;
    r = haeun_net_if_bind(&host->nif, HAEUN_NET_HOST_TX_PA, HAEUN_NET_HOST_RX_PA,
                          HAEUN_NET_HOST_RX_CAP);
    if (r != HAEUN_NET_OK)
        return r;
    /*
     * process-v1 trap only ACKs timer (cause 16). RX IRQ (cause 21) would be
     * mis-dispatched as a syscall and storm. Host poll owns RX; keep NIC on
     * without the RX IRQ enable bit.
     */
    vm->net_control = 1u;
    r = haeun_socks_init(&host->ss, &host->nif);
    if (r != HAEUN_NET_OK)
        return r;
    host->sock_ctx.ss = &host->ss;
    memcpy(host->sock_ctx.next_hop_mac, host->peer_mac, 6u);
    host->mux.sock_ctx = &host->sock_ctx;
    host->mux.gfx_ctx = NULL;
    host->mux.proc_ft = NULL;
    haeun_vm_set_ecall_io(vm, haeun_ecall_mux_io, &host->mux);
    haeun_vm_set_fault_io(vm, haeun_ecall_mux_on_fault, &host->mux);
    if (host->real_mode) {
        if (haeun_net_backend_init(&host->be, host->mac, host->peer_mac,
                                   0x0a000002u) != HAEUN_NET_BE_OK)
            return HAEUN_NET_BAD_ARGUMENT;
    }
    host->attached = 1;
    return HAEUN_NET_OK;
}

/**
 * Binds the virtual NIC and installs mux→socket on ecall_io (loopback).
 */
haeun_net_result haeun_vm_net_host_attach(haeun_vm_net_host *host,
                                          haeun_vm *vm) {
    return haeun_vm_net_host_attach_ex(host, vm, 0);
}

/** Shuts down the host backend and clears attachment flags (idempotent). */
void haeun_vm_net_host_shutdown(haeun_vm_net_host *host) {
    if (host == NULL)
        return;
    if (host->real_mode)
        haeun_net_backend_shutdown(&host->be);
    host->attached = 0;
}

/**
 * Runs the guest; loopback injects one UDP reply, real mode polls host RX.
 */
haeun_vm_result haeun_vm_net_host_run(haeun_vm_net_host *host, haeun_vm *vm,
                                      uint64_t max_steps,
                                      uint64_t *steps_executed) {
    uint8_t frame[HAEUN_NET_FRAME_MAX];
    uint8_t reply[2] = {(uint8_t)'O', (uint8_t)'K'};
    uint32_t flen;
    if (host == NULL || vm == NULL || steps_executed == NULL)
        return HAEUN_VM_BAD_ARGUMENT;
    *steps_executed = 0u;
    while (*steps_executed < max_steps) {
        haeun_vm_result st = haeun_vm_step(vm);
        ++(*steps_executed);
        if (host->real_mode) {
            flen = 0u;
            if (haeun_net_backend_poll_rx(&host->be, frame, sizeof(frame),
                                          &flen) == HAEUN_NET_BE_OK &&
                flen != 0u) {
                if (haeun_vm_net_inject(vm, frame, flen)) {
                    (void)haeun_net_poll(&host->nif);
                    host->injected = 1;
                    (void)printf("net-host: real RX inject len=%u\n", flen);
                }
            }
        } else if (host->tx.count > 0 && !host->injected) {
            flen = build_udp_reply(frame, host->peer_mac, host->mac, 0x0a000001u,
                                   0x0a000002u, 9u, 7u, reply, 2u);
            if (haeun_vm_net_inject(vm, frame, flen)) {
                (void)haeun_net_poll(&host->nif);
                host->injected = 1;
                (void)printf("net-host: injected UDP reply after TX len=%u\n",
                             host->tx.len);
            }
        }
        if (st == HAEUN_VM_HALTED)
            return st;
        if (st != HAEUN_VM_OK)
            return st;
    }
    return HAEUN_VM_STEP_LIMIT;
}
