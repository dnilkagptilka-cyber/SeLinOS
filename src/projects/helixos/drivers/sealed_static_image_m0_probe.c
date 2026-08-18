// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_sealed_static_image_m0_protocol.h"

static int call_validate(seL4_Word expected_status)
{
    seL4_MessageInfo_t reply;

    seL4_SetMR(0, SELINOS_SEALED_STATIC_IMAGE_M0_VALIDATE_QUERY);
    seL4_SetMR(1, SELINOS_SEALED_STATIC_IMAGE_M0_PLAN_SLOT);
    reply = seL4_Call(SELINOS_SEALED_STATIC_IMAGE_M0_PROBE_ENDPOINT_SLOT,
                      seL4_MessageInfo_new(0u, 0u, 0u,
                                           SELINOS_SEALED_STATIC_IMAGE_M0_REQUEST_WORDS));
    return seL4_MessageInfo_get_label(reply) != 0u ||
           seL4_MessageInfo_get_length(reply) != SELINOS_SEALED_STATIC_IMAGE_M0_REPLY_WORDS ||
           seL4_GetMR(0) != expected_status ||
           seL4_GetMR(1) != SELINOS_SEALED_STATIC_IMAGE_M0_PLAN_SLOT ||
           seL4_GetMR(2) != SELINOS_SEALED_STATIC_IMAGE_M0_NEGATIVE_GUARDS;
}

int main(void)
{
    if (call_validate(SELINOS_SEALED_STATIC_IMAGE_M0_ACCEPTED) != 0 ||
        call_validate(SELINOS_SEALED_STATIC_IMAGE_M0_REJECTED) != 0) {
        seL4_DebugPutString("SeLinOS sealed static-image M0 probe: status ledger mismatch.\n");
        return 1;
    }
    seL4_Signal(SELINOS_SEALED_STATIC_IMAGE_M0_PROBE_SUCCESS_NOTIFY_SLOT);
    seL4_DebugPutString("SeLinOS sealed static-image M0 probe: one sealed RX source ledger accepted; duplicate rejected.\n");
    return 0;
}
