#include "haeun/board_mmio.h"
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

enum {
    TX_PA = 0x2000u,
    RX_PA = 0x3000u,
    RX_CAP = 512u
};

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

static void build_arp_req(uint8_t *f, const uint8_t *sha, uint32_t spa_be,
                          uint32_t tpa_be) {
    uint8_t bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t zero[6] = {0, 0, 0, 0, 0, 0};
    haeun_net_eth_write_hdr(f, bcast, sha, HAEUN_ETH_TYPE_ARP);
    f[14] = 0;
    f[15] = 1;
    f[16] = 0x08;
    f[17] = 0x00;
    f[18] = 6;
    f[19] = 4;
    f[20] = 0;
    f[21] = 1;
    memcpy(f + 22, sha, 6);
    f[28] = (uint8_t)(spa_be >> 24);
    f[29] = (uint8_t)(spa_be >> 16);
    f[30] = (uint8_t)(spa_be >> 8);
    f[31] = (uint8_t)spa_be;
    memcpy(f + 32, zero, 6);
    f[38] = (uint8_t)(tpa_be >> 24);
    f[39] = (uint8_t)(tpa_be >> 16);
    f[40] = (uint8_t)(tpa_be >> 8);
    f[41] = (uint8_t)tpa_be;
}

/**
 * Guest TX DMA delivers the exact frame bytes to the host sink.
 */
static int test_nic_tx(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    tx_cap cap;
    uint8_t frame[64];
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    memset(frame, 0xab, sizeof(frame));
    frame[12] = 0x08;
    frame[13] = 0x00;
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_board_maps(HAEUN_NET_BASE));
    haeun_board_write(&vm, HAEUN_NET_BASE + 4u, 1u);
    memcpy(ram + TX_PA, frame, 64u);
    haeun_board_write(&vm, HAEUN_NET_BASE + 16u, TX_PA);
    haeun_board_write(&vm, HAEUN_NET_BASE + 20u, 64u);
    haeun_board_write(&vm, HAEUN_NET_BASE + 24u, 1u);
    CHECK(cap.count == 1);
    CHECK(cap.len == 64u);
    CHECK(memcmp(cap.buf, frame, 64u) == 0);
    return 0;
}

/**
 * Inject copies a frame into guest RX DMA; STATUS reflects pending.
 */
static int test_nic_rx_inject(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    uint8_t frame[64];
    memset(ram, 0, sizeof(ram));
    memset(frame, 0x11, sizeof(frame));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_board_write(&vm, HAEUN_NET_BASE + 28u, RX_PA);
    haeun_board_write(&vm, HAEUN_NET_BASE + 32u, RX_CAP);
    haeun_board_write(&vm, HAEUN_NET_BASE + 4u, 1u);
    CHECK(haeun_vm_net_inject(&vm, frame, 64u));
    CHECK(haeun_board_read(&vm, HAEUN_NET_BASE) == 1u);
    CHECK(haeun_board_read(&vm, HAEUN_NET_BASE + 36u) == 64u);
    CHECK(memcmp(ram + RX_PA, frame, 64u) == 0);
    haeun_board_write(&vm, HAEUN_NET_BASE + 40u, 1u);
    CHECK(haeun_board_read(&vm, HAEUN_NET_BASE) == 0u);
    return 0;
}

/**
 * ARP request for our IP yields a real reply TX through the NIC.
 */
static int test_arp_reply_path(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t req[42];
    uint32_t our_ip = 0x0a000002u; /* 10.0.0.2 */
    uint32_t peer_ip = 0x0a000001u;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    build_arp_req(req, peer, peer_ip, our_ip);
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, our_ip) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_vm_net_inject(&vm, req, 42u));
    CHECK(haeun_net_poll(&nif) == HAEUN_NET_OK);
    CHECK(cap.count == 1);
    CHECK(cap.len == 42u);
    CHECK(haeun_net_eth_type(cap.buf, cap.len) == HAEUN_ETH_TYPE_ARP);
    CHECK(cap.buf[21] == 2u); /* reply */
    CHECK(memcmp(cap.buf + 22, mac, 6u) == 0);
    CHECK(cap.buf[28] == 0x0au && cap.buf[29] == 0x00u && cap.buf[30] == 0x00u &&
          cap.buf[31] == 0x02u);
    CHECK(memcmp(cap.buf + 32, peer, 6u) == 0);
    CHECK(cap.buf[38] == 0x0au && cap.buf[39] == 0x00u && cap.buf[40] == 0x00u &&
          cap.buf[41] == 0x01u);
    return 0;
}

int main(void) {
    if (test_nic_tx() != 0) return 1;
    if (test_nic_rx_inject() != 0) return 1;
    if (test_arp_reply_path() != 0) return 1;
    puts("all net tests passed");
    return 0;
}
