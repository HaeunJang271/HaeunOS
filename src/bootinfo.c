#include "haeun/bootinfo.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <string.h>

static void write_le16(uint8_t *p, uint16_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
}

static void write_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

static bool valid_info(const haeun_bootinfo *info) {
    return info->flags==0u && info->ram_size!=0u &&
           info->stack_base<info->stack_top &&
           info->stack_top<=info->ram_size &&
           (info->stack_base&UINT32_C(0x0fff))==0u &&
           (info->stack_top&UINT32_C(0x0f))==0u &&
           info->isa_version>=HAEUN_ISA_VERSION &&
           info->isa_version<=HAEUN_ISA_VERSION_4;
}

haeun_bootinfo_result haeun_bootinfo_encode(uint8_t *output, size_t capacity,
                                             const haeun_bootinfo *info) {
    if (output==NULL || info==NULL) return HAEUN_BOOTINFO_BAD_ARGUMENT;
    if (capacity<HAEUN_BOOTINFO_SIZE) return HAEUN_BOOTINFO_BUFFER_TOO_SMALL;
    if (!valid_info(info)) return HAEUN_BOOTINFO_INVALID;
    memset(output,0,HAEUN_BOOTINFO_SIZE);
    output[0]='H'; output[1]='B'; output[2]='I'; output[3]='1';
    write_le16(output+4u,HAEUN_BOOTINFO_VERSION);
    write_le16(output+6u,HAEUN_BOOTINFO_SIZE);
    write_le32(output+8u,info->flags);
    write_le32(output+12u,info->ram_size);
    write_le32(output+16u,info->stack_base);
    write_le32(output+20u,info->stack_top);
    write_le32(output+24u,info->isa_version);
    write_le32(output+28u,0u);
    return HAEUN_BOOTINFO_OK;
}

haeun_bootinfo_result haeun_bootinfo_decode(const uint8_t *input, size_t size,
                                             haeun_bootinfo *info) {
    haeun_bootinfo parsed;
    uint16_t version;
    if (input==NULL || info==NULL) return HAEUN_BOOTINFO_BAD_ARGUMENT;
    if (size<HAEUN_BOOTINFO_SIZE) return HAEUN_BOOTINFO_INVALID;
    if (input[0]!='H' || input[1]!='B' || input[2]!='I' || input[3]!='1' ||
        read_le16(input+6u)!=HAEUN_BOOTINFO_SIZE || read_le32(input+28u)!=0u)
        return HAEUN_BOOTINFO_INVALID;
    version=read_le16(input+4u);
    if (version!=HAEUN_BOOTINFO_VERSION) return HAEUN_BOOTINFO_UNSUPPORTED_VERSION;
    parsed.flags=read_le32(input+8u);
    parsed.ram_size=read_le32(input+12u);
    parsed.stack_base=read_le32(input+16u);
    parsed.stack_top=read_le32(input+20u);
    parsed.isa_version=read_le32(input+24u);
    if (!valid_info(&parsed)) return HAEUN_BOOTINFO_INVALID;
    *info=parsed;
    return HAEUN_BOOTINFO_OK;
}
