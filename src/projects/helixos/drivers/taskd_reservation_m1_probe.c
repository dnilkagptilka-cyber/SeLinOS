// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_taskd_reservation_m1_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;

    seL4_SetMR(0, SELINOS_TASKD_RES_M1_RESERVE_BUNDLES);
    seL4_SetMR(1, SELINOS_TASKD_RES_M1_SLOT);
    seL4_Send(SELINOS_TASKD_RES_M1_PROBE_CLIENT_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_RES_M1_REQUEST_WORDS));
    (void)seL4_Wait(SELINOS_TASKD_RES_M1_PROBE_SUCCESS_NOTIFY_SLOT, &badge);

    seL4_DebugPutString("SeLinOS taskd reservation M1 probe: combined status-only reservation passed.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
