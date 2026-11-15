#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))

static void put_insn(uint8_t *ram,uint32_t insn) {
    ram[0]=(uint8_t)insn; ram[1]=(uint8_t)(insn>>8);
    ram[2]=(uint8_t)(insn>>16); ram[3]=(uint8_t)(insn>>24);
}

static haeun_vm_result mmio_store(haeun_vm *vm,uint8_t *ram,uint32_t base,
                                  uint32_t offset,uint32_t value) {
    put_insn(ram,ENC(9,2,1,0,offset));
    vm->pc=0u; vm->r[1]=base; vm->r[2]=value;
    return haeun_vm_step(vm);
}

static haeun_vm_result mmio_load(haeun_vm *vm,uint8_t *ram,uint32_t base,
                                 uint32_t offset,uint32_t *value) {
    put_insn(ram,ENC(8,3,1,0,offset));
    vm->pc=0u; vm->r[1]=base;
    haeun_vm_result result=haeun_vm_step(vm);
    *value=vm->r[3];
    return result;
}

static int test_block(void) {
    uint8_t ram[4096],disk[1024],ram_snapshot[4096],disk_snapshot[1024];
    haeun_vm vm; uint32_t value;
    for (size_t i=0u;i<sizeof(disk);++i) disk[i]=(uint8_t)((i*3u)^0x5au);
    memset(ram,0,sizeof(ram));
    CHECK(haeun_vm_init(&vm,ram,sizeof(ram),0u)==HAEUN_VM_OK);
    CHECK(!haeun_vm_block_attach(&vm,NULL,sizeof(disk)));
    CHECK(!haeun_vm_block_attach(&vm,disk,sizeof(disk)-1u));
    CHECK(haeun_vm_block_attach(&vm,disk,sizeof(disk)));

    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,8u,1u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,12u,512u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,4u,1u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_BLOCK_BASE,0u,&value)==HAEUN_VM_OK&&value==2u);
    CHECK(memcmp(ram+512u,disk+512u,HAEUN_BLOCK_SIZE)==0);

    for (size_t i=0u;i<HAEUN_BLOCK_SIZE;++i) ram[512u+i]=(uint8_t)(0xa5u^i);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,8u,0u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,4u,2u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_BLOCK_BASE,0u,&value)==HAEUN_VM_OK&&value==2u);
    CHECK(memcmp(disk,ram+512u,HAEUN_BLOCK_SIZE)==0);

    memcpy(ram_snapshot,ram,sizeof(ram)); memcpy(disk_snapshot,disk,sizeof(disk));
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,12u,513u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,4u,1u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_BLOCK_BASE,0u,&value)==HAEUN_VM_OK&&value==3u);
    CHECK(memcmp(ram_snapshot+512u,ram+512u,HAEUN_BLOCK_SIZE)==0);
    CHECK(memcmp(disk_snapshot,disk,sizeof(disk))==0);

    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,16u,UINT32_MAX)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,12u,4096u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_BLOCK_BASE,4u,1u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_BLOCK_BASE,0u,&value)==HAEUN_VM_OK&&value==3u);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,0u,&value)==HAEUN_VM_OK&&value==4u);
    CHECK(mmio_store(&vm,ram,HAEUN_IRQC_BASE,12u,2u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,0u,&value)==HAEUN_VM_OK&&value==0u);

    put_insn(ram,ENC(8,3,1,0,0)); vm.pc=0u; vm.r[1]=HAEUN_BLOCK_BASE+1u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ALIGNMENT_FAULT&&vm.cause==2u);
    return 0;
}

static int test_timer_irqc(void) {
    uint8_t ram[64];
    haeun_vm vm; uint32_t value;
    memset(ram,0,sizeof(ram));
    CHECK(haeun_vm_init(&vm,ram,sizeof(ram),0u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_TIMER_BASE,8u,1u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_TIMER_BASE,16u,1u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_TIMER_BASE,16u,&value)==HAEUN_VM_OK&&value==0u);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,0u,&value)==HAEUN_VM_OK&&value==1u);
    CHECK(mmio_store(&vm,ram,HAEUN_IRQC_BASE,4u,1u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,8u,&value)==HAEUN_VM_OK&&value==0u);
    CHECK(mmio_store(&vm,ram,HAEUN_IRQC_BASE,12u,0u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,0u,&value)==HAEUN_VM_OK&&value==0u);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,8u,&value)==HAEUN_VM_OK&&
          value==UINT32_C(0xffffffff));
    return 0;
}

static int test_uart_pending(void) {
    uint8_t ram[32];
    haeun_vm vm; uint32_t value;
    uint8_t byte=0x41;
    memset(ram,0,sizeof(ram));
    CHECK(haeun_vm_init(&vm,ram,sizeof(ram),0u)==HAEUN_VM_OK);
    CHECK(mmio_store(&vm,ram,HAEUN_UART_BASE,8u,1u)==HAEUN_VM_OK);
    CHECK(haeun_vm_uart_feed(&vm,&byte,1u));
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,0u,&value)==HAEUN_VM_OK&&value==2u);
    CHECK(mmio_store(&vm,ram,HAEUN_IRQC_BASE,4u,2u)==HAEUN_VM_OK);
    CHECK(mmio_load(&vm,ram,HAEUN_IRQC_BASE,8u,&value)==HAEUN_VM_OK&&value==1u);
    return 0;
}

int main(void) {
    if (test_block()!=0) return 1;
    if (test_timer_irqc()!=0) return 1;
    if (test_uart_pending()!=0) return 1;
    puts("all block-device MMIO tests passed");
    return 0;
}
