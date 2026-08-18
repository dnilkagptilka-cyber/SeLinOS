// SPDX-License-Identifier: MIT
#ifndef SELINOS_SEALED_STATIC_IMAGE_M0_H
#define SELINOS_SEALED_STATIC_IMAGE_M0_H

#include "selinos_kabi_module.h"

#define SELINOS_SEALED_STATIC_IMAGE_M0_BYTES 4096u
#define SELINOS_SEALED_STATIC_IMAGE_M0_MAGIC_BYTES 8u
#define SELINOS_SEALED_STATIC_IMAGE_M0_VERSION 1u
#define SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES 64u
#define SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_BYTES 32u
#define SELINOS_SEALED_STATIC_IMAGE_M0_SEGMENT_COUNT 1u
#define SELINOS_SEALED_STATIC_IMAGE_M0_TABLE_OFFSET 64u
#define SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_OFFSET 32u
#define SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES 32u
#define SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR 0x60000000UL
#define SELINOS_SEALED_STATIC_IMAGE_M0_PAGE_BYTES 4096u
#define SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_READ 0x1u
#define SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_WRITE 0x2u
#define SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_EXECUTE 0x4u
#define SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX \
    (SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_READ | \
     SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_EXECUTE)

enum selinos_sealed_static_image_m0_status {
    SELINOS_SEALED_STATIC_IMAGE_M0_OK = 0,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_ARGUMENT = -1,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_HEADER = -2,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE = -3,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS = -4,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION = -5,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST = -6,
    SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED = -7,
};

struct selinos_sealed_static_image_m0_summary {
    selinos_u64 entry_vaddr;
    selinos_u32 source_bytes;
    selinos_u32 segment_file_offset;
    selinos_u32 segment_file_bytes;
    selinos_u32 segment_memory_bytes;
    unsigned short permissions;
    selinos_u8 payload_sha256[SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES];
};

/* Validates one caller-supplied SSIM-v1 byte buffer. It allocates nothing,
 * creates no seL4 objects, maps no pages, changes no permissions, and retains
 * no pointer into the supplied image after return. */
int selinos_sealed_static_image_m0_parse(
    const selinos_u8 *image, selinos_size_t image_bytes,
    struct selinos_sealed_static_image_m0_summary *summary);

/* Produces the deterministic M0 test byte stream. The result remains ordinary
 * parser input; this helper maps nothing and is not a loader. */
void selinos_sealed_static_image_m0_make_fixture(
    selinos_u8 image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES]);

#endif
