// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_opaque_lease_m1_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_OPAQUE_LEASE_M1_OBJECT_ENDPOINT_SLOT,
                                           &badge);

    if (seL4_MessageInfo_get_label(message) != 0u ||
        seL4_MessageInfo_get_length(message) != SELINOS_OPAQUE_LEASE_M1_REQUEST_WORDS ||
        seL4_GetMR(0) != SELINOS_OPAQUE_LEASE_M1_REQUEST ||
        seL4_GetMR(1) != SELINOS_OPAQUE_LEASE_M1_SLOT) {
        seL4_DebugPutString("SeLinOS objectd opaque lease M1: malformed taskd request rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    seL4_SetMR(0, SELINOS_OPAQUE_LEASE_M1_GRANTED);
    seL4_SetCap(0, SELINOS_OPAQUE_LEASE_M1_OBJECT_TOKEN_SOURCE_SLOT);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, SELINOS_OPAQUE_LEASE_M1_REPLY_CAPS,
                                    SELINOS_OPAQUE_LEASE_M1_REPLY_WORDS));
    seL4_DebugPutString("SeLinOS objectd opaque lease M1: one notification token transferred to taskd.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
