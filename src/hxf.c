#include "haeun/hxf.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <string.h>

#define HXF_HEADER_SIZE 64u
#define HXF_SEGMENT_SIZE 32u
#define HXF_MAX_SEGMENTS 16u
#define HXF_USER_LIMIT UINT32_C(0xc0000000)

typedef struct {
    uint32_t flags, file_offset, virtual_address, file_size, memory_size, alignment;
} hxf_segment;

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store_le16(uint8_t *p, uint16_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value >> 8);
}

static void store_le32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value >> 8);
    p[2]=(uint8_t)(value >> 16); p[3]=(uint8_t)(value >> 24);
}

static bool zero_bytes(const uint8_t *p, size_t length) {
    size_t i;
    for (i = 0u; i < length; ++i) if (p[i] != 0u) return false;
    return true;
}

static bool ranges_overlap(uint64_t a, uint64_t a_size,
                           uint64_t b, uint64_t b_size) {
    return a < b + b_size && b < a + a_size;
}

haeun_hxf_result haeun_hxf_write(const haeun_hxf_segment *segments,
                                 uint16_t segment_count, uint32_t entry_address,
                                 uint8_t *output, size_t output_capacity,
                                 size_t *output_size) {
    uint64_t offsets[HXF_MAX_SEGMENTS], cursor;
    uint16_t i, j;
    bool entry_valid=false;
    if (segments==NULL || output_size==NULL || (output==NULL && output_capacity!=0u))
        return HAEUN_HXF_BAD_ARGUMENT;
    if (segment_count==0u || segment_count>HXF_MAX_SEGMENTS)
        return HAEUN_HXF_INVALID_IMAGE;
    cursor=(uint64_t)HXF_HEADER_SIZE+(uint64_t)segment_count*HXF_SEGMENT_SIZE;
    for (i=0u;i<segment_count;++i) {
        const haeun_hxf_segment *s=&segments[i];
        uint64_t memory_end;
        if ((s->flags&~7u)!=0u || s->file_size>s->memory_size ||
            (s->file_size!=0u && s->file_data==NULL) || s->alignment<4u ||
            (s->alignment&(s->alignment-1u))!=0u ||
            s->virtual_address%s->alignment!=0u)
            return HAEUN_HXF_INVALID_IMAGE;
        memory_end=(uint64_t)s->virtual_address+s->memory_size;
        if (memory_end>HXF_USER_LIMIT) return HAEUN_HXF_INVALID_IMAGE;
        for (j=0u;j<i;++j) {
            const haeun_hxf_segment *prior=&segments[j];
            if (ranges_overlap(s->virtual_address,s->memory_size,
                               prior->virtual_address,prior->memory_size))
                return HAEUN_HXF_INVALID_IMAGE;
        }
        if (s->memory_size!=0u && (s->flags&4u)!=0u &&
            (uint64_t)entry_address>=s->virtual_address &&
            (uint64_t)entry_address<memory_end) entry_valid=true;
        cursor=(cursor+(uint64_t)s->alignment-1u)&~((uint64_t)s->alignment-1u);
        offsets[i]=cursor;
        cursor+=s->file_size;
        if (cursor>UINT32_MAX) return HAEUN_HXF_INVALID_IMAGE;
    }
    if ((entry_address&3u)!=0u || !entry_valid) return HAEUN_HXF_INVALID_IMAGE;
    *output_size=(size_t)cursor;
    if (output==NULL || output_capacity<cursor) return HAEUN_HXF_BUFFER_TOO_SMALL;

    memset(output,0,(size_t)cursor);
    output[0]='H'; output[1]='X'; output[2]='F'; output[3]='1';
    store_le16(output+4u,HXF_HEADER_SIZE); store_le16(output+6u,1u);
    store_le32(output+8u,HAEUN_ISA_VERSION); store_le32(output+16u,entry_address);
    store_le32(output+20u,HXF_HEADER_SIZE); store_le16(output+24u,segment_count);
    store_le16(output+26u,HXF_SEGMENT_SIZE); store_le32(output+28u,(uint32_t)cursor);
    for (i=0u;i<segment_count;++i) {
        const haeun_hxf_segment *s=&segments[i];
        uint8_t *raw=output+HXF_HEADER_SIZE+(size_t)i*HXF_SEGMENT_SIZE;
        store_le32(raw,1u); store_le32(raw+4u,s->flags);
        store_le32(raw+8u,(uint32_t)offsets[i]);
        store_le32(raw+12u,s->virtual_address); store_le32(raw+16u,s->file_size);
        store_le32(raw+20u,s->memory_size); store_le32(raw+24u,s->alignment);
        if (s->file_size!=0u) memcpy(output+(size_t)offsets[i],s->file_data,s->file_size);
    }
    return HAEUN_HXF_OK;
}

haeun_hxf_result haeun_hxf_load_ex(const uint8_t *image, size_t image_size,
                                   uint8_t *ram, size_t ram_size,
                                   uint32_t *entry_address, uint32_t *isa_version) {
    hxf_segment segments[HXF_MAX_SEGMENTS];
    uint32_t entry, table_offset, declared_size, image_isa;
    uint16_t count, entry_size;
    uint64_t table_end;
    size_t i, j;
    bool entry_valid = false;

    if (image == NULL || ram == NULL || entry_address == NULL || isa_version == NULL)
        return HAEUN_HXF_BAD_ARGUMENT;
    if (image_size < HXF_HEADER_SIZE || image_size > UINT32_MAX)
        return HAEUN_HXF_INVALID_IMAGE;
    if (image[0] != 'H' || image[1] != 'X' || image[2] != 'F' || image[3] != '1')
        return HAEUN_HXF_INVALID_IMAGE;
    if (read_le16(image + 4u) != HXF_HEADER_SIZE)
        return HAEUN_HXF_INVALID_IMAGE;
    image_isa=read_le32(image+8u);
    if (read_le16(image + 6u) != 1u || image_isa<HAEUN_ISA_VERSION ||
        image_isa>HAEUN_ISA_VERSION_4)
        return HAEUN_HXF_UNSUPPORTED_VERSION;
    if (read_le32(image + 12u) != 0u || !zero_bytes(image + 32u, 32u))
        return HAEUN_HXF_INVALID_IMAGE;

    entry = read_le32(image + 16u);
    table_offset = read_le32(image + 20u);
    count = read_le16(image + 24u);
    entry_size = read_le16(image + 26u);
    declared_size = read_le32(image + 28u);
    if (count == 0u || count > HXF_MAX_SEGMENTS || entry_size != HXF_SEGMENT_SIZE ||
        declared_size != image_size || table_offset < HXF_HEADER_SIZE)
        return HAEUN_HXF_INVALID_IMAGE;
    table_end = (uint64_t)table_offset + (uint64_t)count * HXF_SEGMENT_SIZE;
    if (table_end > image_size) return HAEUN_HXF_INVALID_IMAGE;

    for (i = 0u; i < count; ++i) {
        const uint8_t *raw = image + table_offset + i * HXF_SEGMENT_SIZE;
        uint32_t type = read_le32(raw);
        hxf_segment *s = &segments[i];
        uint64_t file_end, memory_end;
        s->flags = read_le32(raw + 4u);
        s->file_offset = read_le32(raw + 8u);
        s->virtual_address = read_le32(raw + 12u);
        s->file_size = read_le32(raw + 16u);
        s->memory_size = read_le32(raw + 20u);
        s->alignment = read_le32(raw + 24u);
        if (type != 1u || (s->flags & ~7u) != 0u || read_le32(raw + 28u) != 0u ||
            s->file_size > s->memory_size || s->alignment < 4u ||
            (s->alignment & (s->alignment - 1u)) != 0u ||
            s->file_offset % s->alignment != 0u ||
            s->virtual_address % s->alignment != 0u)
            return HAEUN_HXF_INVALID_IMAGE;
        file_end = (uint64_t)s->file_offset + s->file_size;
        memory_end = (uint64_t)s->virtual_address + s->memory_size;
        if (file_end > image_size || memory_end > HXF_USER_LIMIT ||
            memory_end > ram_size ||
            (s->file_size != 0u && ranges_overlap(s->file_offset, s->file_size,
                                                   0u, HXF_HEADER_SIZE)) ||
            (s->file_size != 0u && ranges_overlap(s->file_offset, s->file_size,
                                                   table_offset, table_end - table_offset)))
            return HAEUN_HXF_INVALID_IMAGE;
        for (j = 0u; j < i; ++j) {
            const hxf_segment *prior = &segments[j];
            if (ranges_overlap(s->virtual_address, s->memory_size,
                               prior->virtual_address, prior->memory_size) ||
                (s->file_size != 0u && prior->file_size != 0u &&
                 ranges_overlap(s->file_offset, s->file_size,
                                prior->file_offset, prior->file_size)))
                return HAEUN_HXF_INVALID_IMAGE;
        }
        if (s->memory_size != 0u && (s->flags & 4u) != 0u &&
            (uint64_t)entry >= s->virtual_address && (uint64_t)entry < memory_end)
            entry_valid = true;
    }
    if ((entry & 3u) != 0u || !entry_valid) return HAEUN_HXF_INVALID_IMAGE;

    /* All validation is complete; only now mutate guest RAM. */
    for (i = 0u; i < count; ++i) {
        const hxf_segment *s = &segments[i];
        memcpy(ram + s->virtual_address, image + s->file_offset, s->file_size);
        memset(ram + s->virtual_address + s->file_size, 0,
               (size_t)(s->memory_size - s->file_size));
    }
    *entry_address = entry;
    *isa_version=image_isa;
    return HAEUN_HXF_OK;
}

haeun_hxf_result haeun_hxf_load(const uint8_t *image, size_t image_size,
                                uint8_t *ram, size_t ram_size,
                                uint32_t *entry_address) {
    uint32_t isa_version;
    if (image!=NULL && image_size>=12u && image[0]=='H' && image[1]=='X' &&
        image[2]=='F' && image[3]=='1' && read_le16(image+4u)==64u &&
        read_le16(image+6u)==1u && read_le32(image+8u)!=HAEUN_ISA_VERSION)
        return HAEUN_HXF_UNSUPPORTED_VERSION;
    return haeun_hxf_load_ex(image,image_size,ram,ram_size,entry_address,&isa_version);
}
