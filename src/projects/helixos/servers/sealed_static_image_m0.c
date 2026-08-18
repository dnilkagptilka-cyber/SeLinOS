// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sel4/sel4.h>

#include "selinos_sealed_static_image_m0.h"
#include "selinos_sealed_static_image_m0_protocol.h"

static void reply(seL4_Word status)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_SEALED_STATIC_IMAGE_M0_PLAN_SLOT);
    seL4_SetMR(2, SELINOS_SEALED_STATIC_IMAGE_M0_NEGATIVE_GUARDS);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,
                                    SELINOS_SEALED_STATIC_IMAGE_M0_REPLY_WORDS));
}

static bool negative_guards_pass(uint8_t image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES])
{
    uint8_t mutated[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
    struct selinos_sealed_static_image_m0_summary summary;

    if (selinos_sealed_static_image_m0_parse(image, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_OK) {
        return false;
    }
    memcpy(mutated, image, sizeof(mutated));
    mutated[96u] ^= 1u;
    if (selinos_sealed_static_image_m0_parse(mutated, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST) {
        return false;
    }
    memcpy(mutated, image, sizeof(mutated));
    mutated[84u] = SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX |
                   SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_WRITE;
    if (selinos_sealed_static_image_m0_parse(mutated, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION) {
        return false;
    }
    memcpy(mutated, image, sizeof(mutated));
    mutated[24u] = 0u;
    mutated[25u] = 0u;
    mutated[26u] = 0u;
    mutated[27u] = 0u;
    mutated[28u] = 0u;
    mutated[29u] = 0x80u;
    mutated[30u] = 0u;
    mutated[31u] = 0u;
    if (selinos_sealed_static_image_m0_parse(mutated, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS) {
        return false;
    }
    memcpy(mutated, image, sizeof(mutated));
    mutated[86u] = 1u;
    if (selinos_sealed_static_image_m0_parse(mutated, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED) {
        return false;
    }
    memcpy(mutated, image, sizeof(mutated));
    mutated[72u] = 98u;
    mutated[76u] = 2u;
    mutated[80u] = 2u;
    if (selinos_sealed_static_image_m0_parse(mutated, sizeof(mutated), &summary) !=
        SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE) {
        return false;
    }
    return true;
}

int main(void)
{
    bool queried = false;
    for (;;) {
        uint8_t image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
        seL4_Word badge = 0u;
        seL4_MessageInfo_t message = seL4_Recv(
            SELINOS_SEALED_STATIC_IMAGE_M0_SERVER_ENDPOINT_SLOT, &badge);
        bool exact_query = seL4_MessageInfo_get_label(message) == 0u &&
                           seL4_MessageInfo_get_length(message) ==
                               SELINOS_SEALED_STATIC_IMAGE_M0_REQUEST_WORDS &&
                           seL4_GetMR(0) ==
                               SELINOS_SEALED_STATIC_IMAGE_M0_VALIDATE_QUERY &&
                           seL4_GetMR(1) == SELINOS_SEALED_STATIC_IMAGE_M0_PLAN_SLOT;
        (void)badge;
        if (!exact_query || queried) {
            reply(SELINOS_SEALED_STATIC_IMAGE_M0_REJECTED);
            continue;
        }
        selinos_sealed_static_image_m0_make_fixture(image);
        if (!negative_guards_pass(image)) {
            seL4_DebugPutString("SeLinOS sealed static-image M0: parser guard mismatch.\n");
            reply(SELINOS_SEALED_STATIC_IMAGE_M0_REJECTED);
            continue;
        }
        queried = true;
        reply(SELINOS_SEALED_STATIC_IMAGE_M0_ACCEPTED);
        seL4_DebugPutString("SeLinOS sealed static-image M0: one SSIM-v1 RX ledger accepted; digest, W+X, canonical, reserved and range guards rejected before mapping.\n");
        seL4_DebugPutString("SeLinOS sealed static-image M0: parser-only status transaction; no frame, mapping, TCB, task resume or ELF claim.\n");
    }
}
