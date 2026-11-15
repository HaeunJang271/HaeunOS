#include "haeun/bootinfo.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

int main(void) {
    uint8_t bytes[HAEUN_BOOTINFO_SIZE], damaged[HAEUN_BOOTINFO_SIZE];
    haeun_bootinfo info={0u,UINT32_C(0x10000000),UINT32_C(0x0ff00000),
                         UINT32_C(0x10000000),3u};
    haeun_bootinfo decoded={0};
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_OK);
    CHECK(memcmp(bytes,"HBI1",4u)==0);
    CHECK(bytes[4]==1u && bytes[5]==0u && bytes[6]==32u && bytes[7]==0u);
    CHECK(read_le32(bytes+12u)==UINT32_C(0x10000000));
    CHECK(read_le32(bytes+16u)==UINT32_C(0x0ff00000));
    CHECK(read_le32(bytes+20u)==UINT32_C(0x10000000));
    CHECK(read_le32(bytes+24u)==3u && read_le32(bytes+28u)==0u);
    CHECK(haeun_bootinfo_decode(bytes,sizeof(bytes),&decoded)==HAEUN_BOOTINFO_OK);
    CHECK(decoded.flags==0u && decoded.ram_size==info.ram_size &&
          decoded.stack_base==info.stack_base && decoded.stack_top==info.stack_top &&
          decoded.isa_version==info.isa_version);
    memset(damaged,0x5a,sizeof(damaged));
    CHECK(haeun_bootinfo_encode(damaged,sizeof(damaged)-1u,&info)==HAEUN_BOOTINFO_BUFFER_TOO_SMALL);
    CHECK(damaged[0]==0x5au && damaged[31]==0x5au);
    CHECK(haeun_bootinfo_encode(NULL,sizeof(bytes),&info)==HAEUN_BOOTINFO_BAD_ARGUMENT);
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),NULL)==HAEUN_BOOTINFO_BAD_ARGUMENT);
    CHECK(haeun_bootinfo_decode(bytes,sizeof(bytes)-1u,&decoded)==HAEUN_BOOTINFO_INVALID);
    memcpy(damaged,bytes,sizeof(bytes)); damaged[0]='?';
    CHECK(haeun_bootinfo_decode(damaged,sizeof(damaged),&decoded)==HAEUN_BOOTINFO_INVALID);
    memcpy(damaged,bytes,sizeof(bytes)); damaged[4]=2u;
    CHECK(haeun_bootinfo_decode(damaged,sizeof(damaged),&decoded)==HAEUN_BOOTINFO_UNSUPPORTED_VERSION);
    memcpy(damaged,bytes,sizeof(bytes)); damaged[28]=1u;
    CHECK(haeun_bootinfo_decode(damaged,sizeof(damaged),&decoded)==HAEUN_BOOTINFO_INVALID);
    decoded.ram_size=77u;
    memcpy(damaged,bytes,sizeof(bytes)); damaged[8]=1u;
    CHECK(haeun_bootinfo_decode(damaged,sizeof(damaged),&decoded)==HAEUN_BOOTINFO_INVALID);
    CHECK(decoded.ram_size==77u);
    info.stack_base=info.stack_top;
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_INVALID);
    info.stack_base=UINT32_C(0x0ff00001);
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_INVALID);
    info.stack_base=UINT32_C(0x0ff00000); info.stack_top=UINT32_C(0x10000001);
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_INVALID);
    info.stack_top=UINT32_C(0x10000000); info.flags=1u;
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_INVALID);
    info.flags=0u; info.isa_version=5u;
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_INVALID);
    info.isa_version=4u;
    CHECK(haeun_bootinfo_encode(bytes,sizeof(bytes),&info)==HAEUN_BOOTINFO_OK);
    puts("all HBI boot-information tests passed");
    return 0;
}
