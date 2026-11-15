#include "haeun/net.h"
#include "haeun/net_sock.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define ENC(op, rd, ra, rb, imm)                                               \
    (((uint32_t)(op) << 26) | ((uint32_t)(rd) << 22) | ((uint32_t)(ra) << 18) | \
     ((uint32_t)(rb) << 14) | ((uint32_t)(imm) & 0x3ffffu))

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum { TX_PA = 0x2000u, RX_PA = 0x3000u, RX_CAP = 512u, PAY_VA = 0x400u };

typedef struct {
    uint8_t buf[HAEUN_NET_FRAME_MAX];
    uint32_t len;
    int count;
} tx_cap;

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void on_tx(void *ctx, const uint8_t *frame, uint32_t length) {
    tx_cap *c = (tx_cap *)ctx;
    if (c == NULL || frame == NULL || length > HAEUN_NET_FRAME_MAX) return;
    memcpy(c->buf, frame, length);
    c->len = length;
    c->count++;
}

/**
 * Guest ECALL SOCKET/BIND/SENDTO completes via host assist and TX capture.
 */
static int test_ecall_udp_sendto(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_net_if nif;
    haeun_net_socks ss;
    haeun_sock_ecall_ctx ctx;
    tx_cap cap;
    uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t peer[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint32_t fd;
    memset(ram, 0, sizeof(ram));
    memset(&cap, 0, sizeof(cap));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0)); /* ECALL */
    put_le32(ram + 4u, ENC(0x01, 0, 0, 0, 0)); /* HALT */
    ram[PAY_VA] = 'h';
    ram[PAY_VA + 1u] = 'i';
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    haeun_vm_net_set_tx_sink(&vm, on_tx, &cap);
    CHECK(haeun_net_if_init(&nif, &vm, mac, 0x0a000002u) == HAEUN_NET_OK);
    CHECK(haeun_net_if_bind(&nif, TX_PA, RX_PA, RX_CAP) == HAEUN_NET_OK);
    CHECK(haeun_socks_init(&ss, &nif) == HAEUN_NET_OK);
    memset(&ctx, 0, sizeof(ctx));
    ctx.ss = &ss;
    memcpy(ctx.next_hop_mac, peer, 6u);
    haeun_vm_set_ecall_io(&vm, haeun_sock_ecall_io, &ctx);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_SOCKET;
    vm.r[2] = (uint32_t)HAEUN_SOCK_DGRAM;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 4u);
    fd = vm.r[1];
    CHECK(fd < HAEUN_NET_MAX_SOCKS);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_BIND;
    vm.r[2] = fd;
    vm.r[3] = 7u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_SENDTO;
    vm.r[2] = fd;
    vm.r[3] = 0x0a000001u;
    vm.r[4] = 9u;
    vm.r[5] = PAY_VA;
    vm.r[6] = 2u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    CHECK(cap.count == 1);
    CHECK(cap.len > 14u);
    return 0;
}

/**
 * Unknown sock syscall numbers still go through sec poison + trap.
 */
static int test_unknown_still_traps(void) {
    uint8_t ram[64];
    haeun_vm vm;
    haeun_sock_ecall_ctx ctx;
    haeun_net_socks ss;
    memset(ram, 0, sizeof(ram));
    memset(&ctx, 0, sizeof(ctx));
    memset(&ss, 0, sizeof(ss));
    ctx.ss = &ss;
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.csr_trapvec = 32u;
    haeun_vm_set_ecall_io(&vm, haeun_sock_ecall_io, &ctx);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = 99u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 32u && vm.r[1] == 0xffffffffu);
    return 0;
}

int main(void) {
    if (test_ecall_udp_sendto() != 0) return 1;
    if (test_unknown_still_traps() != 0) return 1;
    puts("all net sock ecall tests passed");
    return 0;
}
