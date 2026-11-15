#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while (0)

static void put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void make_image(uint8_t *b) {
    memset(b,0,100u); memcpy(b,"HXF1",4u);
    put16(b+4u,64u); put16(b+6u,1u); put32(b+8u,1u);
    put32(b+16u,0x1000u); put32(b+20u,64u); put16(b+24u,1u);
    put16(b+26u,32u); put32(b+28u,100u);
    put32(b+64u,1u); put32(b+68u,5u); put32(b+72u,96u);
    put32(b+76u,0x1000u); put32(b+80u,4u); put32(b+84u,8u);
    put32(b+88u,4u); b[96]=0x11u; b[97]=0x22u; b[98]=0x33u; b[99]=0x44u;
}

int main(void) {
    uint8_t image[100], ram[8192]; uint32_t entry=0u, isa=0u; haeun_vm vm;
    uint64_t steps=0u;
    uint8_t code[4]={0u,0u,0u,4u}; size_t written=0u;
    haeun_hxf_segment segment={5u,0x1000u,code,4u,8u,4u};
    make_image(image);
    CHECK(haeun_hxf_write(&segment,1u,0x1000u,image,sizeof(image),&written)==HAEUN_HXF_OK);
    CHECK(written==sizeof(image));
    CHECK(haeun_hxf_load(image,written,ram,sizeof(ram),&entry)==HAEUN_HXF_OK);
    CHECK(ram[0x1000u]==0u && ram[0x1003u]==4u && ram[0x1007u]==0u);
    memset(image,0x5a,sizeof(image)); written=0u;
    CHECK(haeun_hxf_write(&segment,1u,0x1000u,image,sizeof(image)-1u,&written)==HAEUN_HXF_BUFFER_TOO_SMALL);
    CHECK(written==sizeof(image) && image[0]==0x5au && image[99]==0x5au);

    make_image(image); memset(ram,0xa5,sizeof(ram));
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_OK);
    CHECK(entry==0x1000u && ram[0x1000u]==0x11u && ram[0x1003u]==0x44u);
    CHECK(ram[0x1004u]==0u && ram[0x1007u]==0u && ram[0x0fffu]==0xa5u);

    make_image(image); put32(image+80u,4u); put32(image+84u,4u);
    put32(image+96u,UINT32_C(0x04000000));
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_OK);
    CHECK(haeun_vm_init(&vm,ram,sizeof(ram),entry)==HAEUN_VM_OK);
    CHECK(haeun_vm_run(&vm,8u,&steps)==HAEUN_VM_HALTED);
    CHECK(steps==1u);

    make_image(image); put32(image+8u,2u); put32(image+96u,UINT32_C(0x04000000));
    memset(ram,0xa5,sizeof(ram));
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_UNSUPPORTED_VERSION);
    CHECK(ram[0x1000u]==0xa5u);
    CHECK(haeun_hxf_load_ex(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HXF_OK);
    CHECK(isa==2u && haeun_vm_init_version(&vm,ram,sizeof(ram),entry,isa)==HAEUN_VM_OK);
    CHECK(haeun_vm_run(&vm,8u,&steps)==HAEUN_VM_HALTED);

    make_image(image); put32(image+8u,3u); put32(image+96u,UINT32_C(0x04000000));
    CHECK(haeun_hxf_load_ex(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HXF_OK);
    CHECK(isa==3u && haeun_vm_init_version(&vm,ram,sizeof(ram),entry,isa)==HAEUN_VM_OK);
    CHECK(haeun_vm_run(&vm,8u,&steps)==HAEUN_VM_HALTED);

    make_image(image); memset(ram,0xa5,sizeof(ram)); entry=123u; image[0]='?';
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    CHECK(ram[0x1000u]==0xa5u && entry==123u);
    make_image(image); put32(image+8u,5u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_UNSUPPORTED_VERSION);
    make_image(image); put32(image+8u,4u); put32(image+96u,UINT32_C(0x04000000));
    CHECK(haeun_hxf_load_ex(image,sizeof(image),ram,sizeof(ram),&entry,&isa)==HAEUN_HXF_OK);
    CHECK(isa==4u && haeun_vm_init_version(&vm,ram,sizeof(ram),entry,isa)==HAEUN_VM_OK);
    CHECK(haeun_vm_run(&vm,8u,&steps)==HAEUN_VM_HALTED);
    make_image(image); put32(image+68u,0x10u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    make_image(image); put32(image+16u,0x1008u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    make_image(image); put32(image+76u,0xfffffffcu); put32(image+84u,16u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    make_image(image); put32(image+72u,64u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    make_image(image); put32(image+80u,9u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    make_image(image); put32(image+28u,99u);
    CHECK(haeun_hxf_load(image,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_INVALID_IMAGE);
    CHECK(haeun_hxf_load(NULL,sizeof(image),ram,sizeof(ram),&entry)==HAEUN_HXF_BAD_ARGUMENT);
    segment.virtual_address=0x1004u;
    CHECK(haeun_hxf_write(&segment,1u,0x1000u,image,sizeof(image),&written)==HAEUN_HXF_INVALID_IMAGE);
    CHECK(get32(image+16u)==0x1000u);
    puts("all HXF validation and loading tests passed"); return 0;
}
