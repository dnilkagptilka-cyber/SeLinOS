// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_tcb_resume_m2_protocol.h"

int main(void)
{
    seL4_SetMR(0, SELINOS_TCB_RESUME_M2_CHILD_RAN);
    seL4_SetMR(1, SELINOS_TCB_RESUME_M2_SLOT);
    seL4_SetMR(2, SELINOS_TCB_RESUME_M2_GENERATION);
    seL4_Send(SELINOS_TCB_RESUME_M2_CHILD_COMPLETION_SEND_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TCB_RESUME_M2_CHILD_COMPLETION_WORDS));
    seL4_DebugPutString("SeLinOS TCB resume M2 child: one fixed completion sent.\n");
    for (;;) { (void)seL4_Yield(); }
}
