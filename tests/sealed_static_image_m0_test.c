// SPDX-License-Identifier: MIT
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selinos_sealed_static_image_m0.h"

static int expect_status(const char *name, const uint8_t *image, size_t image_bytes,
                         int expected)
{
    struct selinos_sealed_static_image_m0_summary summary;
    int actual = selinos_sealed_static_image_m0_parse(image, image_bytes, &summary);

    if (actual != expected) {
        fprintf(stderr, "%s: expected %d, got %d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

static void put_u16(uint8_t *image, size_t offset, uint16_t value)
{
    image[offset] = (uint8_t)value;
    image[offset + 1u] = (uint8_t)(value >> 8u);
}

static void put_u32(uint8_t *image, size_t offset, uint32_t value)
{
    image[offset] = (uint8_t)value;
    image[offset + 1u] = (uint8_t)(value >> 8u);
    image[offset + 2u] = (uint8_t)(value >> 16u);
    image[offset + 3u] = (uint8_t)(value >> 24u);
}

static void put_u64(uint8_t *image, size_t offset, uint64_t value)
{
    size_t index;

    for (index = 0u; index < 8u; index++) {
        image[offset + index] = (uint8_t)(value >> (index * 8u));
    }
}

int main(int argc, char **argv)
{
    uint8_t image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
    uint8_t mutated[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
    struct selinos_sealed_static_image_m0_summary summary;
    FILE *fixture;
    size_t read_bytes;

    if (argc != 2) {
        fprintf(stderr, "usage: %s fixture.ssim\n", argv[0]);
        return 2;
    }
    fixture = fopen(argv[1], "rb");
    if (fixture == NULL) {
        perror("fopen");
        return 2;
    }
    read_bytes = fread(image, 1u, sizeof(image), fixture);
    if (fclose(fixture) != 0 || read_bytes != sizeof(image)) {
        fprintf(stderr, "fixture must be exactly one 4 KiB image\n");
        return 2;
    }
    if (selinos_sealed_static_image_m0_parse(image, sizeof(image), &summary) !=
            SELINOS_SEALED_STATIC_IMAGE_M0_OK ||
        summary.entry_vaddr != SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR ||
        summary.segment_file_bytes != 3u ||
        summary.permissions != SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX) {
        fprintf(stderr, "valid fixture rejected or summary mismatch\n");
        return 1;
    }

    memcpy(mutated, image, sizeof(mutated));
    mutated[96] ^= 0x01u;
    if (expect_status("altered-payload", mutated, sizeof(mutated),
                      SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST) != 0) {
        return 1;
    }
    memcpy(mutated, image, sizeof(mutated));
    put_u16(mutated, 84u, SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX |
                           SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_WRITE);
    if (expect_status("writable-executable", mutated, sizeof(mutated),
                      SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION) != 0) {
        return 1;
    }
    memcpy(mutated, image, sizeof(mutated));
    put_u64(mutated, 24u, UINT64_C(0x0000800000000000));
    if (expect_status("non-canonical-entry", mutated, sizeof(mutated),
                      SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS) != 0) {
        return 1;
    }
    memcpy(mutated, image, sizeof(mutated));
    put_u16(mutated, 86u, 1u);
    if (expect_status("reserved-field", mutated, sizeof(mutated),
                      SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED) != 0) {
        return 1;
    }
    memcpy(mutated, image, sizeof(mutated));
    put_u32(mutated, 72u, 98u);
    put_u32(mutated, 76u, 2u);
    put_u32(mutated, 80u, 2u);
    if (expect_status("source-range", mutated, sizeof(mutated),
                      SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE) != 0) {
        return 1;
    }
    puts("SeLinOS sealed static-image M0 host parser: valid ledger plus all negative guards passed.");
    return 0;
}
