// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_opaque_lease_m1_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;

    seL4_SetMR(0, SELINOS_OPAQUE_LEASE_M1_REQUEST);
    seL4_SetMR(1, SELINOS_OPAQUE_LEASE_M1_SLOT);
    seL4_Send(SELINOS_OPAQUE_LEASE_M1_PROBE_CLIENT_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_OPAQUE_LEASE_M1_REQUEST_WORDS));
    (void)seL4_Wait(SELINOS_OPAQUE_LEASE_M1_PROBE_SUCCESS_NOTIFY_SLOT, &badge);

    seL4_DebugPutString("SeLinOS taskd opaque lease M1 probe: fixed token transfer passed.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
