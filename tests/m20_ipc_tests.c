/**
 * M20.2: IPC send/recv, FIFO, isolation, overflow, peek, reap/reuse, VA deny.
 */
#include "haeun/guest_pcb.h"
#include "haeun/ipc.h"
#include "haeun/m20_ecall.h"
#include "haeun/register_file.h"
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

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    put_le32(ram + 0xA400u, curr);
    put_le32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

static int test_send_recv_fifo(void) {
    haeun_ipc_hub hub;
    uint8_t a[4] = {1, 2, 3, 4}, b[4] = {9, 8, 7, 6}, out[8];
    int epid, sender = -1, n;
    haeun_ipc_hub_init(&hub);
    epid = haeun_ipc_create(&hub, 2);
    CHECK(epid >= 0);
    CHECK(haeun_ipc_send(&hub, epid, 3, a, 4) == 0);
    CHECK(haeun_ipc_send(&hub, epid, 4, b, 4) == 0);
    CHECK(haeun_ipc_available(&hub, epid, 2) == 2);
    n = haeun_ipc_recv(&hub, epid, 2, out, 8, &sender);
    CHECK(n == 4 && sender == 3 && out[0] == 1 && out[3] == 4);
    n = haeun_ipc_recv(&hub, epid, 2, out, 8, &sender);
    CHECK(n == 4 && sender == 4 && out[0] == 9);
    CHECK(haeun_ipc_recv(&hub, epid, 2, out, 8, &sender) == 0);
    return 0;
}

static int test_sender_kernel_and_isolation(void) {
    haeun_ipc_hub hub;
    haeun_ipc_msg peek;
    uint8_t msg[4] = {0xAAu, 0, 0, 0}, out[4];
    int epid, sender = 99;
    haeun_ipc_hub_init(&hub);
    epid = haeun_ipc_create(&hub, 5);
    CHECK(haeun_ipc_send(&hub, epid, 7, msg, 1) == 0);
    CHECK(haeun_ipc_peek(&hub, epid, 5, &peek) == 1);
    CHECK(peek.sender_pid == 7);
    CHECK(peek.receiver_pid == 5);
    CHECK(peek.type == 0xAAu);
    CHECK(peek.seq == 0u);
    CHECK(haeun_ipc_recv(&hub, epid, 6, out, 4, &sender) < 0);
    CHECK(haeun_ipc_available(&hub, epid, 6) < 0);
    CHECK(haeun_ipc_recv(&hub, epid, 5, out, 4, &sender) == 1);
    CHECK(sender == 7);
    return 0;
}

static int test_bad_args_and_overflow(void) {
    haeun_ipc_hub hub;
    uint8_t msg[40], out[8];
    int epid, i, sender;
    haeun_ipc_hub_init(&hub);
    epid = haeun_ipc_create(&hub, 1);
    memset(msg, 0x11, sizeof(msg));
    CHECK(haeun_ipc_send(&hub, epid, 2, msg, 0) == -3);
    CHECK(haeun_ipc_send(&hub, epid, 2, msg, 40) == -3);
    CHECK(haeun_ipc_send(&hub, 99, 2, msg, 4) == -1);
    CHECK(haeun_ipc_send(&hub, epid, 0, msg, 4) == -1);
    for (i = 0; i < HAEUN_IPC_QUEUE; ++i)
        CHECK(haeun_ipc_send(&hub, epid, 2, msg, 4) == 0);
    CHECK(haeun_ipc_send(&hub, epid, 2, msg, 4) == -2);
    CHECK(hub.eps[epid].drops == 1u);
    CHECK(haeun_ipc_recv(&hub, epid, 1, out, 8, &sender) == 4);
    return 0;
}

static int test_reap_reuse(void) {
    haeun_ipc_hub hub;
    uint8_t msg[2] = {1, 2}, out[4];
    int epid, sender = -1;
    haeun_ipc_hub_init(&hub);
    epid = haeun_ipc_create(&hub, 9);
    CHECK(haeun_ipc_send(&hub, epid, 3, msg, 2) == 0);
    haeun_ipc_reap(&hub, 9);
    CHECK(haeun_ipc_recv(&hub, epid, 9, out, 4, &sender) < 0);
    epid = haeun_ipc_create(&hub, 9);
    CHECK(epid >= 0);
    CHECK(haeun_ipc_available(&hub, epid, 9) == 0);
    CHECK(haeun_ipc_recv(&hub, epid, 9, out, 4, &sender) == 0);
    return 0;
}

static int test_ecall_va_and_poll(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_ipc_hub hub;
    haeun_m20_ecall_ctx ctx;
    int epid;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    haeun_ipc_hub_init(&hub);
    memset(&ctx, 0, sizeof(ctx));
    ctx.ipc = &hub;
    haeun_vm_set_ecall_io(&vm, haeun_m20_ecall_io, &ctx);
    set_pcb_pid(ram, 1u, 4u);
    vm.privilege = HAEUN_PRIV_USER;
    epid = haeun_ipc_create(&hub, 4);
    CHECK(epid >= 0);
    /* bad VA on SEND */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_IPC_SEND;
    vm.r[2] = (uint32_t)epid;
    vm.r[3] = 0xfffffff0u;
    vm.r[4] = 4u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    /* oversize */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_IPC_SEND;
    vm.r[3] = 0x100u;
    put_le32(ram + 0x100u, 0x11223344u);
    vm.r[4] = 100u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xfffffffeu);
    /* good send + available + peek */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_IPC_SEND;
    vm.r[4] = 4u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_IPC_AVAILABLE;
    vm.r[2] = (uint32_t)epid;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 1u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_IPC_PEEK;
    vm.r[2] = (uint32_t)epid;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 4u);
    CHECK(vm.r[2] == 4u); /* sender = current pid */
    return 0;
}

static int test_pingpong_seq(void) {
    haeun_ipc_hub hub;
    uint8_t ping[12], pong[8], out[12];
    int a, b, sender, i;
    haeun_ipc_hub_init(&hub);
    a = haeun_ipc_create(&hub, 10);
    b = haeun_ipc_create(&hub, 11);
    for (i = 0; i < 3; ++i) {
        memset(ping, 0, sizeof(ping));
        ping[0] = 1;
        put_le32(ping + 4, (uint32_t)i);
        put_le32(ping + 8, (uint32_t)b);
        CHECK(haeun_ipc_send(&hub, a, 11, ping, 12) == 0);
        CHECK(haeun_ipc_recv(&hub, a, 10, out, 12, &sender) == 12);
        CHECK(sender == 11);
        memset(pong, 0, sizeof(pong));
        pong[0] = 2;
        put_le32(pong + 4, (uint32_t)i);
        CHECK(haeun_ipc_send(&hub, b, 10, pong, 8) == 0);
        CHECK(haeun_ipc_recv(&hub, b, 11, out, 12, &sender) == 8);
        CHECK(out[0] == 2 && sender == 10);
        CHECK(out[4] == (uint8_t)i);
    }
    return 0;
}

int main(void) {
    if (test_send_recv_fifo() != 0)
        return 1;
    if (test_sender_kernel_and_isolation() != 0)
        return 1;
    if (test_bad_args_and_overflow() != 0)
        return 1;
    if (test_reap_reuse() != 0)
        return 1;
    if (test_ecall_va_and_poll() != 0)
        return 1;
    if (test_pingpong_seq() != 0)
        return 1;
    (void)printf("m20_ipc_tests ok\n");
    return 0;
}
