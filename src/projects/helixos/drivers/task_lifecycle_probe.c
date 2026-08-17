// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_taskd_m1_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;

    seL4_SetMR(0, SELINOS_TASKD_M1_START);
    seL4_SetMR(1, SELINOS_TASKD_M1_CHILD_ID);
    seL4_Send(SELINOS_TASKD_M1_PROBE_CLIENT_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_M1_CLIENT_REQUEST_WORDS));

    (void)seL4_Wait(SELINOS_TASKD_M1_PROBE_SUCCESS_WAIT_SLOT, &badge);
    seL4_DebugPutString("SeLinOS task lifecycle M1 probe: fixed child taskd lifecycle passed.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
