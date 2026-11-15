#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))
#define CSR_IMM(funct,csr) (((uint32_t)(funct)<<12)|((uint32_t)(csr)&0xfffu))

static void put_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static int test_csr_eret_and_ecall_trap(void) {
    uint8_t ram[256];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    /* 0: CSRRW trapvec from r1; 4: ECALL; 8: handler loads cause; 12: HALT */
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,1)));
    put_le32(ram+4u, ENC(0x0d,0,0,0,0));
    put_le32(ram+8u, ENC(0x10,2,0,0,CSR_IMM(2,3)));
    put_le32(ram+12u, ENC(0x01,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=8u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.csr_trapvec==8u&&vm.pc==4u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(vm.pc==8u&&vm.csr_cause==3u&&vm.csr_epc==4u);
    CHECK(vm.privilege==HAEUN_PRIV_SUPERVISOR);
    CHECK((vm.csr_status&1u)==0u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[2]==3u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

static int test_user_csr_privilege_fault(void) {
    uint8_t ram[64];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,1)));
    put_le32(ram+4u, ENC(0x10,2,0,0,CSR_IMM(2,0)));
    put_le32(ram+8u, ENC(0x01,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=8u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    vm.privilege=HAEUN_PRIV_USER;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(vm.pc==8u&&vm.csr_cause==4u&&vm.csr_epc==4u);
    CHECK(vm.privilege==HAEUN_PRIV_SUPERVISOR);
    return 0;
}

static int test_eret_restores_user(void) {
    uint8_t ram[64];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    /* trapvec=16: ERET; user site at 4 continues to HALT */
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,1))); /* trapvec */
    put_le32(ram+4u, ENC(0x0d,0,0,0,0));             /* ECALL from user */
    put_le32(ram+8u, ENC(0x01,0,0,0,0));             /* user resume HALT */
    put_le32(ram+16u, ENC(0x11,0,0,0,0));            /* ERET */
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=16u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    vm.privilege=HAEUN_PRIV_USER;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* ECALL -> handler */
    CHECK(vm.pc==16u&&vm.csr_epc==4u);
    CHECK(((vm.csr_status>>2)&1u)==HAEUN_PRIV_USER);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* ERET */
    CHECK(vm.pc==4u&&vm.privilege==HAEUN_PRIV_USER);
    /* Re-enter by forcing supervisor path: set epc to 8 and ERET from S */
    vm.privilege=HAEUN_PRIV_SUPERVISOR;
    vm.csr_epc=8u;
    vm.csr_status=0u; /* PP=0, PIE=0 */
    put_le32(ram+20u, ENC(0x11,0,0,0,0));
    vm.pc=20u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
    CHECK(vm.pc==8u&&vm.privilege==HAEUN_PRIV_USER);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

static int test_timer_interrupt_delivery(void) {
    uint8_t ram[128];
    haeun_vm vm;
    uint32_t status_ie=1u;
    memset(ram,0,sizeof(ram));
    /*
     * 0: CSRRW trapvec=r1
     * 4: SW compare=1
     * 8: SW timer control=1  (fires on retire when count>=1)
     * 12: SW irq enable=1
     * 16: CSRRW status IE=1  -> after retire, interrupt to handler
     * 20: should not run (NOP)
     * 32: handler CSRRS cause -> r2; HALT
     */
    put_le32(ram+0u, ENC(0x10,0,1,0,CSR_IMM(1,1)));
    put_le32(ram+4u, ENC(9,3,4,0,8));   /* sw r3, 8(r4) compare_lo */
    put_le32(ram+8u, ENC(9,5,4,0,16));  /* sw r5, 16(r4) control */
    put_le32(ram+12u, ENC(9,6,7,0,4));  /* sw r6, 4(r7) irq enable */
    put_le32(ram+16u, ENC(0x10,0,8,0,CSR_IMM(1,0))); /* status = r8 */
    put_le32(ram+20u, ENC(0,0,0,0,0));
    put_le32(ram+32u, ENC(0x10,2,0,0,CSR_IMM(2,3)));
    put_le32(ram+36u, ENC(0x01,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_4)==HAEUN_VM_OK);
    vm.r[1]=32u;
    vm.r[3]=1u;
    vm.r[4]=HAEUN_TIMER_BASE;
    vm.r[5]=1u;
    vm.r[6]=1u;
    vm.r[7]=HAEUN_IRQC_BASE;
    vm.r[8]=status_ie;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* trapvec */
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* compare */
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* timer enable/fire */
    CHECK((vm.irq_pending&1u)!=0u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* irq enable */
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); /* IE=1 -> interrupt */
    CHECK(vm.pc==32u&&vm.csr_cause==16u&&vm.csr_epc==20u);
    CHECK((vm.csr_status&1u)==0u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[2]==16u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    return 0;
}

static int test_v3_rejects_csr(void) {
    uint8_t ram[16];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    put_le32(ram, ENC(0x10,0,1,0,CSR_IMM(1,1)));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_3)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION&&vm.cause==0u);
    return 0;
}

static int test_v3_rejects_sfence(void) {
    uint8_t ram[16];
    haeun_vm vm;
    memset(ram,0,sizeof(ram));
    put_le32(ram, ENC(0x12,0,0,0,0));
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_3)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION&&vm.cause==0u);
    return 0;
}

int main(void) {
    if (test_csr_eret_and_ecall_trap()!=0) return 1;
    if (test_user_csr_privilege_fault()!=0) return 1;
    if (test_eret_restores_user()!=0) return 1;
    if (test_timer_interrupt_delivery()!=0) return 1;
    if (test_v3_rejects_csr()!=0) return 1;
    if (test_v3_rejects_sfence()!=0) return 1;
    puts("all ISA v4 privilege and interrupt tests passed");
    return 0;
}
