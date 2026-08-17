// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_tcb_resume_m2_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TCB_RESUME_M2_OBJECT_ENDPOINT_SLOT, &badge);
    if (seL4_MessageInfo_get_label(message) != 0u ||
        seL4_MessageInfo_get_length(message) != SELINOS_TCB_RESUME_M2_REQUEST_WORDS ||
        seL4_GetMR(0) != SELINOS_TCB_RESUME_M2_REQUEST ||
        seL4_GetMR(1) != SELINOS_TCB_RESUME_M2_SLOT) {
        seL4_DebugPutString("SeLinOS objectd TCB resume M2: malformed taskd request rejected.\n");
        for (;;) { (void)seL4_Yield(); }
    }
    seL4_SetMR(0, SELINOS_TCB_RESUME_M2_GRANTED);
    seL4_SetCap(0, SELINOS_TCB_RESUME_M2_OBJECT_TCB_SOURCE_SLOT);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, SELINOS_TCB_RESUME_M2_REPLY_CAPS,
                                    SELINOS_TCB_RESUME_M2_REPLY_WORDS));
    seL4_DebugPutString("SeLinOS objectd TCB resume M2: suspended child TCB cap transferred to taskd.\n");
    for (;;) { (void)seL4_Yield(); }
}
