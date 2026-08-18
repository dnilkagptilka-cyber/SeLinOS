// SPDX-License-Identifier: MIT
#include "selinos_kabi_policy.h"
#include "selinos_sealed_static_image_m0.h"

static void zero_bytes(selinos_u8 *destination, selinos_size_t bytes)
{
    selinos_size_t index;

    for (index = 0u; index < bytes; index++) {
        destination[index] = 0u;
    }
}

static void write_bytes(selinos_u8 *destination, const selinos_u8 *source,
                        selinos_size_t bytes)
{
    selinos_size_t index;

    for (index = 0u; index < bytes; index++) {
        destination[index] = source[index];
    }
}

static void write_u16(selinos_u8 *output, unsigned short value)
{
    output[0] = (selinos_u8)value;
    output[1] = (selinos_u8)(value >> 8u);
}

static void write_u32(selinos_u8 *output, selinos_u32 value)
{
    output[0] = (selinos_u8)value;
    output[1] = (selinos_u8)(value >> 8u);
    output[2] = (selinos_u8)(value >> 16u);
    output[3] = (selinos_u8)(value >> 24u);
}

static void write_u64(selinos_u8 *output, selinos_u64 value)
{
    selinos_u32 index;

    for (index = 0u; index < 8u; index++) {
        output[index] = (selinos_u8)(value >> (index * 8u));
    }
}

void selinos_sealed_static_image_m0_make_fixture(
    selinos_u8 image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES])
{
    static const selinos_u8 magic[SELINOS_SEALED_STATIC_IMAGE_M0_MAGIC_BYTES] = {
        'S', 'E', 'L', 'I', 'N', 'S', '6', '4',
    };
    const selinos_u32 source_bytes = SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES +
                                     SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_BYTES +
                                     3u;
    const selinos_u32 payload_offset = SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES +
                                       SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_BYTES;

    zero_bytes(image, SELINOS_SEALED_STATIC_IMAGE_M0_BYTES);
    write_bytes(image, magic, sizeof(magic));
    write_u16(image + 8u, SELINOS_SEALED_STATIC_IMAGE_M0_VERSION);
    write_u16(image + 10u, SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES);
    write_u16(image + 12u, SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_COUNT);
    write_u32(image + 16u, SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET);
    write_u32(image + 20u, source_bytes);
    write_u64(image + 24u, SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR);
    write_u64(image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET,
              SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR);
    write_u32(image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET + 8u,
              payload_offset);
    write_u32(image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET + 12u, 3u);
    write_u32(image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET + 16u, 3u);
    write_u16(image + SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET + 20u,
              SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX);
    image[payload_offset] = 0x90u;
    image[payload_offset + 1u] = 0x0fu;
    image[payload_offset + 2u] = 0x0bu;
    selinos_kabi_sha256(image, source_bytes,
                        image + SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_OFFSET);
}
