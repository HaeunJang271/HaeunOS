#include "haeun/hfw.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

static void put16(uint8_t *p,uint16_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
}
static void put32(uint8_t *p,uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static void make_image(uint8_t *image) {
    memset(image,0,36u);
    memcpy(image,"HFW1",4u); put16(image+4u,1u); put16(image+6u,32u);
    put32(image+8u,3u); put32(image+20u,4u); put32(image+24u,8u);
    put32(image+32u,UINT32_C(0x04000000));
}

int main(void) {
    uint8_t image[36],trailing[37],ram[64]; uint32_t entry=99u,isa=99u;
    make_image(image); memset(ram,0xa5,sizeof(ram));
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_OK);
    CHECK(entry==0u && isa==3u);
    CHECK(ram[0]==0u && ram[1]==0u && ram[2]==0u && ram[3]==4u);
    CHECK(ram[4]==0u && ram[7]==0u && ram[8]==0xa5u);

    entry=99u; isa=99u; memset(ram,0xa5,sizeof(ram));
    CHECK(haeun_hfw_load(image,sizeof(image)-1u,ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    CHECK(entry==99u && isa==99u && ram[0]==0xa5u);
    memcpy(trailing,image,sizeof(image)); trailing[36]=0u;
    CHECK(haeun_hfw_load(trailing,sizeof(trailing),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    CHECK(haeun_hfw_load(NULL,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_BAD_ARGUMENT);
    CHECK(haeun_hfw_load(image,sizeof(image),NULL,sizeof(ram),&entry,&isa)==HAEUN_HFW_BAD_ARGUMENT);

    make_image(image); image[0]='?';
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put16(image+4u,2u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_UNSUPPORTED_VERSION);
    make_image(image); put16(image+6u,28u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+8u,5u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_UNSUPPORTED_VERSION);
    make_image(image); put32(image+8u,4u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_OK&&isa==4u);
    make_image(image); put32(image+12u,4u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+16u,4u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+20u,3u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+20u,0u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+24u,0u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+24u,3u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+24u,UINT32_C(0x10000)+4u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image); put32(image+28u,1u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_INVALID_IMAGE);
    make_image(image);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,4u,&entry,&isa)==HAEUN_HFW_RAM_TOO_SMALL);
    make_image(image); put32(image+8u,1u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_OK);
    CHECK(isa==1u && entry==0u);
    make_image(image); put32(image+8u,2u);
    CHECK(haeun_hfw_load(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HFW_OK);
    CHECK(isa==2u && entry==0u);
    puts("all HFW validation and staging tests passed");
    return 0;
}
