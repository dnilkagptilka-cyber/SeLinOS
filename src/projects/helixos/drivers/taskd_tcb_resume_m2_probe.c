// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_tcb_resume_m2_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;
    seL4_SetMR(0, SELINOS_TCB_RESUME_M2_REQUEST);
    seL4_SetMR(1, SELINOS_TCB_RESUME_M2_SLOT);
    seL4_Send(SELINOS_TCB_RESUME_M2_PROBE_CLIENT_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TCB_RESUME_M2_REQUEST_WORDS));
    (void)seL4_Wait(SELINOS_TCB_RESUME_M2_PROBE_SUCCESS_NOTIFY_SLOT, &badge);
    seL4_DebugPutString("SeLinOS taskd TCB resume M2 probe: one transferred child resume passed.\n");
    for (;;) { (void)seL4_Yield(); }
}
