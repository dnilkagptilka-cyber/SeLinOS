// SPDX-License-Identifier: MIT
#include "selinos_kabi_policy.h"
#include "selinos_sealed_static_image_m0.h"

static const selinos_u8 ssiv_magic[SELINOS_SEALED_STATIC_IMAGE_M0_MAGIC_BYTES] = {
    'S', 'E', 'L', 'I', 'N', 'S', '6', '4',
};

static unsigned short read_u16(const selinos_u8 *input)
{
    return (unsigned short)input[0] | ((unsigned short)input[1] << 8u);
}

static selinos_u32 read_u32(const selinos_u8 *input)
{
    return (selinos_u32)input[0] | ((selinos_u32)input[1] << 8u) |
           ((selinos_u32)input[2] << 16u) | ((selinos_u32)input[3] << 24u);
}

static selinos_u64 read_u64(const selinos_u8 *input)
{
    selinos_u64 value = 0u;
    selinos_size_t index;

    for (index = 0u; index < 8u; index++) {
        value |= (selinos_u64)input[index] << (index * 8u);
    }
    return value;
}

static int checked_range(selinos_u32 offset, selinos_u32 bytes, selinos_u32 limit)
{
    return offset <= limit && bytes <= limit - offset;
}

static int exact_bytes(const selinos_u8 *left, const selinos_u8 *right,
                       selinos_size_t bytes)
{
    selinos_size_t index;

    for (index = 0u; index < bytes; index++) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static void copy_bytes(selinos_u8 *destination, const selinos_u8 *source,
                       selinos_size_t bytes)
{
    selinos_size_t index;

    for (index = 0u; index < bytes; index++) {
        destination[index] = source[index];
    }
}

static void zero_bytes(selinos_u8 *destination, selinos_size_t bytes)
{
    selinos_size_t index;

    for (index = 0u; index < bytes; index++) {
        destination[index] = 0u;
    }
}

int selinos_sealed_static_image_m0_parse(
    const selinos_u8 *image, selinos_size_t image_bytes,
    struct selinos_sealed_static_image_m0_summary *summary)
{
    selinos_u8 zeroed_source[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
    selinos_u8 computed_source_digest[SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES];
    const selinos_u8 *segment;
    selinos_u32 source_bytes;
    selinos_u32 segment_file_offset;
    selinos_u32 segment_file_bytes;
    selinos_u32 segment_memory_bytes;
    unsigned short permissions;
    selinos_u64 entry_vaddr;
    selinos_u64 segment_vaddr;

    if (image == 0 || summary == 0 ||
        image_bytes != SELINOS_SEALED_STATIC_IMAGE_M0_BYTES) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_ARGUMENT;
    }
    if (!exact_bytes(image, ssiv_magic, sizeof(ssiv_magic)) ||
        read_u16(image + 8u) != SELINOS_SEALED_STATIC_IMAGE_M0_VERSION ||
        read_u16(image + 10u) != SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES ||
        read_u16(image + 12u) != SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_COUNT ||
        read_u16(image + 14u) != 0u ||
        read_u32(image + 16u) != SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_HEADER;
    }
    source_bytes = read_u32(image + 20u);
    entry_vaddr = read_u64(image + 24u);
    if (source_bytes < SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES +
                           SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_BYTES ||
        source_bytes > SELINOS_SEALED_STATIC_IMAGE_M0_BYTES) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE;
    }
    if (entry_vaddr != SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR ||
        entry_vaddr % SELINOS_SEALED_STATIC_IMAGE_M0_PAGE_BYTES != 0u) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS;
    }
    segment = image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET;
    segment_vaddr = read_u64(segment);
    segment_file_offset = read_u32(segment + 8u);
    segment_file_bytes = read_u32(segment + 12u);
    segment_memory_bytes = read_u32(segment + 16u);
    permissions = read_u16(segment + 20u);
    if (segment_vaddr != entry_vaddr ||
        segment_file_offset < SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES +
                              SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_BYTES ||
        segment_file_bytes == 0u ||
        segment_file_bytes != segment_memory_bytes ||
        !checked_range(segment_file_offset, segment_file_bytes, source_bytes)) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE;
    }
    if (permissions != SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION;
    }
    if (read_u16(segment + 22u) != 0u || read_u64(segment + 24u) != 0u) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED;
    }
    copy_bytes(zeroed_source, image, source_bytes);
    zero_bytes(zeroed_source + SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_OFFSET,
               SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES);
    selinos_kabi_sha256(zeroed_source, source_bytes, computed_source_digest);
    if (!exact_bytes(computed_source_digest,
                     image + SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_OFFSET,
                     SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES)) {
        return SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST;
    }
    zero_bytes((selinos_u8 *)summary, sizeof(*summary));
    summary->entry_vaddr = entry_vaddr;
    summary->source_bytes = source_bytes;
    summary->segment_file_offset = segment_file_offset;
    summary->segment_file_bytes = segment_file_bytes;
    summary->segment_memory_bytes = segment_memory_bytes;
    summary->permissions = permissions;
    selinos_kabi_sha256(image + segment_file_offset, segment_file_bytes,
                        summary->payload_sha256);
    return SELINOS_SEALED_STATIC_IMAGE_M0_OK;
}
