#include "haeun/hfw.h"
#include "haeun/vm.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

haeun_hfw_result haeun_hfw_load(const uint8_t *image, size_t image_size,
                                uint8_t *ram, size_t ram_size,
                                uint32_t *entry_address, uint32_t *isa_version) {
    uint32_t isa, file_size, memory_size;
    uint64_t expected_size;
    if (image==NULL || ram==NULL || entry_address==NULL || isa_version==NULL)
        return HAEUN_HFW_BAD_ARGUMENT;
    if (image_size<HAEUN_HFW_HEADER_SIZE || image_size>UINT32_MAX ||
        memcmp(image,"HFW1",4u)!=0 || read_le16(image+6u)!=HAEUN_HFW_HEADER_SIZE ||
        read_le32(image+12u)!=0u || read_le32(image+16u)!=0u ||
        read_le32(image+28u)!=0u)
        return HAEUN_HFW_INVALID_IMAGE;
    if (read_le16(image+4u)!=1u) return HAEUN_HFW_UNSUPPORTED_VERSION;
    isa=read_le32(image+8u);
    if (isa<HAEUN_ISA_VERSION || isa>HAEUN_ISA_VERSION_4)
        return HAEUN_HFW_UNSUPPORTED_VERSION;
    file_size=read_le32(image+20u);
    memory_size=read_le32(image+24u);
    expected_size=(uint64_t)HAEUN_HFW_HEADER_SIZE+file_size;
    if (file_size<4u || (file_size&3u)!=0u || memory_size<file_size ||
        memory_size>HAEUN_HFW_MAX_FIRMWARE_SIZE || (memory_size&3u)!=0u ||
        expected_size!=(uint64_t)image_size)
        return HAEUN_HFW_INVALID_IMAGE;
    if ((uint64_t)memory_size>(uint64_t)ram_size)
        return HAEUN_HFW_RAM_TOO_SMALL;

    memcpy(ram,image+HAEUN_HFW_HEADER_SIZE,file_size);
    memset(ram+file_size,0,(size_t)(memory_size-file_size));
    *entry_address=0u;
    *isa_version=isa;
    return HAEUN_HFW_OK;
}
