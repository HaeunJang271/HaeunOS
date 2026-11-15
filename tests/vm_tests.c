#include "haeun/vm.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)
#define ENC(op,rd,ra,rb,imm) (((uint32_t)(op)<<26)|((uint32_t)(rd)<<22)|((uint32_t)(ra)<<18)|((uint32_t)(rb)<<14)|((uint32_t)(imm)&0x3ffffu))

static uint8_t tx_bytes[16];
static size_t tx_count;
static void capture_tx(void *context,uint8_t byte) {
    (void)context;
    if (tx_count<sizeof(tx_bytes)) tx_bytes[tx_count++]=byte;
}
static void put_instruction(uint8_t *ram,size_t offset,uint32_t instruction) {
    ram[offset]=(uint8_t)instruction;
    ram[offset+1u]=(uint8_t)(instruction>>8);
    ram[offset+2u]=(uint8_t)(instruction>>16);
    ram[offset+3u]=(uint8_t)(instruction>>24);
}

static int setup(haeun_vm *vm, uint8_t *ram, uint32_t instruction) {
    memset(ram,0,256u);
    ram[0]=(uint8_t)instruction; ram[1]=(uint8_t)(instruction>>8);
    ram[2]=(uint8_t)(instruction>>16); ram[3]=(uint8_t)(instruction>>24);
    return haeun_vm_init(vm,ram,256u,0u)==HAEUN_VM_OK ? 0 : 1;
}

int main(void) {
    uint8_t ram[256]; haeun_vm vm;
    CHECK(setup(&vm,ram,ENC(0,0,0,0,0))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==4u);
    CHECK(setup(&vm,ram,ENC(1,0,0,0,0))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED); CHECK(vm.halted);
    CHECK(setup(&vm,ram,ENC(2,3,1,2,0))==0); vm.r[1]=7u; vm.r[2]=9u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==16u);
    CHECK(setup(&vm,ram,ENC(3,3,1,2,0))==0); vm.r[1]=9u; vm.r[2]=7u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==2u);
    CHECK(setup(&vm,ram,ENC(4,3,1,2,0))==0); vm.r[1]=6u; vm.r[2]=3u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==2u);
    CHECK(setup(&vm,ram,ENC(5,3,1,2,0))==0); vm.r[1]=4u; vm.r[2]=1u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==5u);
    CHECK(setup(&vm,ram,ENC(6,3,1,2,0))==0); vm.r[1]=6u; vm.r[2]=3u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==5u);
    CHECK(setup(&vm,ram,ENC(7,3,1,0,(uint32_t)-2))==0); vm.r[1]=10u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==8u);
    CHECK(setup(&vm,ram,ENC(7,3,1,0,131071u))==0); vm.r[1]=1u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==131072u);
    CHECK(setup(&vm,ram,ENC(8,3,1,0,4))==0); vm.r[1]=32u; ram[36]=0x78; ram[37]=0x56; ram[38]=0x34; ram[39]=0x12; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[3]==0x12345678u);
    CHECK(setup(&vm,ram,ENC(9,3,1,0,4))==0); vm.r[1]=32u; vm.r[3]=0x12345678u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(ram[36]==0x78&&ram[39]==0x12);
    CHECK(setup(&vm,ram,ENC(10,1,2,0,1))==0); vm.r[1]=5; vm.r[2]=5; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==8u);
    CHECK(setup(&vm,ram,ENC(11,1,2,0,1))==0); vm.r[1]=5; vm.r[2]=5; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==4u);
    CHECK(setup(&vm,ram,ENC(12,5,0,0,2))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[5]==4u&&vm.pc==12u);
    CHECK(setup(&vm,ram,ENC(12,5,0,0,(uint32_t)-1))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[5]==4u&&vm.pc==0u);
    CHECK(setup(&vm,ram,ENC(14,0,1,0,0))==0); vm.r[1]=8u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(setup(&vm,ram,ENC(14,15,1,0,0))==0); ram[11]=4u;
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,HAEUN_ISA_VERSION_2)==HAEUN_VM_OK);
    vm.r[1]=8u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==8u&&vm.r[15]==4u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_HALTED);
    CHECK(setup(&vm,ram,ENC(14,15,1,0,0))==0); CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,2u)==HAEUN_VM_OK);
    vm.r[1]=6u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==6u);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ALIGNMENT_FAULT);
    CHECK(setup(&vm,ram,ENC(14,1,1,0,(uint32_t)-4))==0); ram[11]=4u;
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,2u)==HAEUN_VM_OK);
    vm.r[1]=12u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.pc==8u&&vm.r[1]==4u);
    CHECK(setup(&vm,ram,ENC(15,3,1,2,0u))==0); vm.r[1]=0xffffffffu; vm.r[2]=0u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,2u)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,3u)==HAEUN_VM_OK);
    vm.r[1]=0xffffffffu; vm.r[2]=0u;
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0u);
    for (uint32_t cond=1u;cond<=5u;++cond) {
        CHECK(setup(&vm,ram,ENC(15,3,1,2,cond<<11))==0);
        CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,3u)==HAEUN_VM_OK);
        vm.r[1]=0xffffffffu; vm.r[2]=0u;
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
        CHECK(vm.r[3]==((cond==1u||cond==2u||cond==3u)?1u:0u));
    }
    CHECK(setup(&vm,ram,ENC(15,3,1,2,6u<<11))==0);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,3u)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(setup(&vm,ram,ENC(15,3,1,2,1u))==0);
    CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,3u)==HAEUN_VM_OK);
    CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    {
        static const uint32_t operands[][2]={{0u,0u},{0x7fffffffu,0x80000000u},
            {0x80000000u,0x7fffffffu},{0xffffffffu,0xffffffffu}};
        static const uint32_t expected[][6]={{1u,0u,0u,1u,0u,1u},
            {0u,1u,0u,0u,1u,1u},{0u,1u,1u,1u,0u,0u},
            {1u,0u,0u,1u,0u,1u}};
        for (size_t pair=0;pair<sizeof(operands)/sizeof(operands[0]);++pair) {
            for (uint32_t cond=0;cond<6u;++cond) {
                CHECK(setup(&vm,ram,ENC(15,3,1,2,cond<<11))==0);
                CHECK(haeun_vm_init_version(&vm,ram,sizeof(ram),0u,3u)==HAEUN_VM_OK);
                vm.r[1]=operands[pair][0]; vm.r[2]=operands[pair][1];
                CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK);
                CHECK(vm.r[3]==expected[pair][cond]);
            }
        }
    }
    CHECK(setup(&vm,ram,ENC(13,0,0,0,0))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_TRAP); CHECK(vm.cause==3u&&vm.fault_pc==0u);
    CHECK(setup(&vm,ram,ENC(63,0,0,0,0))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(setup(&vm,ram,ENC(0,0,0,0,0))==0); vm.pc=2u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_ALIGNMENT_FAULT);
    CHECK(setup(&vm,ram,ENC(7,0,0,0,5))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[0]==0u);
    CHECK(setup(&vm,ram,ENC(2,1,1,2,1))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(setup(&vm,ram,ENC(12,1,1,0,1))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_ILLEGAL_INSTRUCTION);
    CHECK(setup(&vm,ram,ENC(8,1,2,0,0))==0); vm.r[2]=3u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_ALIGNMENT_FAULT);
    CHECK(setup(&vm,ram,ENC(8,1,2,0,0))==0); vm.r[2]=256u; CHECK(haeun_vm_step(&vm)==HAEUN_VM_MEMORY_FAULT);
    {
        const uint8_t input[]={0x41u,0x42u};
        CHECK(setup(&vm,ram,ENC(8,3,1,0,0))==0); vm.r[1]=HAEUN_UART_BASE;
        put_instruction(ram,4u,ENC(8,4,1,0,4));
        put_instruction(ram,8u,ENC(8,3,1,0,0));
        put_instruction(ram,12u,ENC(8,3,1,0,0));
        put_instruction(ram,16u,ENC(8,4,1,0,4));
        CHECK(haeun_vm_uart_feed(&vm,input,sizeof(input)));
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0x41u);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[4]==3u);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0x42u);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==UINT32_C(0x80000000));
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[4]==2u);
    }
    {
        static uint8_t full[HAEUN_UART_RX_CAPACITY];
        const uint8_t extra=0x7eu;
        CHECK(setup(&vm,ram,ENC(8,3,1,0,0))==0); vm.r[1]=HAEUN_UART_BASE;
        CHECK(haeun_vm_uart_feed(&vm,full,sizeof(full)));
        CHECK(vm.uart_rx_count==HAEUN_UART_RX_CAPACITY);
        CHECK(!haeun_vm_uart_feed(&vm,&extra,1u));
        CHECK(vm.uart_rx_count==HAEUN_UART_RX_CAPACITY);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0u);
        CHECK(haeun_vm_uart_feed(&vm,&extra,1u));
        CHECK(vm.uart_rx_count==HAEUN_UART_RX_CAPACITY&&vm.uart_rx[0]==extra);
    }
    {
        CHECK(setup(&vm,ram,ENC(9,2,1,0,0))==0); vm.r[1]=HAEUN_UART_BASE;
        vm.r[2]=UINT32_C(0x123456ab); tx_count=0u;
        haeun_vm_uart_set_tx_sink(&vm,capture_tx,NULL);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&tx_count==1u&&tx_bytes[0]==0xabu);
        CHECK(setup(&vm,ram,ENC(9,2,1,0,8))==0); vm.r[1]=HAEUN_UART_BASE; vm.r[2]=3u;
        put_instruction(ram,4u,ENC(8,3,1,0,8));
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.uart_control==1u);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==1u);
        CHECK(setup(&vm,ram,ENC(8,3,1,0,12))==0); vm.r[1]=HAEUN_UART_BASE;
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0u);
        CHECK(setup(&vm,ram,ENC(8,3,1,0,0))==0); vm.r[1]=HAEUN_IRQC_BASE;
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK&&vm.r[3]==0u);
        CHECK(setup(&vm,ram,ENC(8,3,1,0,0))==0); vm.r[1]=UINT32_C(0x10008000);
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_MEMORY_FAULT&&vm.cause==1u);
        CHECK(setup(&vm,ram,ENC(8,3,1,0,0))==0); vm.r[1]=HAEUN_UART_BASE+1u;
        CHECK(haeun_vm_step(&vm)==HAEUN_VM_ALIGNMENT_FAULT&&vm.cause==2u);
    }
    CHECK(setup(&vm,ram,ENC(7,1,0,0,(uint32_t)-131072))==0); CHECK(haeun_vm_step(&vm)==HAEUN_VM_OK); CHECK(vm.r[1]==0xfffe0000u);
    CHECK(setup(&vm,ram,ENC(0,0,0,0,0))==0); { uint64_t n; CHECK(haeun_vm_run(&vm,1u,&n)==HAEUN_VM_STEP_LIMIT); CHECK(n==1u); }
    CHECK(setup(&vm,ram,ENC(1,0,0,0,0))==0); { uint64_t n; CHECK(haeun_vm_run(&vm,1u,&n)==HAEUN_VM_HALTED); CHECK(n==1u); }
    puts("all VM instruction and fault tests passed"); return 0;
}
