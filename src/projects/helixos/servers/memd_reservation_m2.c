// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_taskd_reservation_m1_protocol.h"

int main(void)
{
    seL4_Word badge = 0u;
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TASKD_RES_M1_MEM_SERVER_ENDPOINT_SLOT,
                                           &badge);

    if (seL4_MessageInfo_get_label(message) != 0u ||
        seL4_MessageInfo_get_length(message) != SELINOS_TASKD_RES_M1_REQUEST_WORDS ||
        seL4_GetMR(0) != SELINOS_TASKD_RES_M1_MEM_RESERVE ||
        seL4_GetMR(1) != SELINOS_TASKD_RES_M1_SLOT) {
        seL4_DebugPutString("SeLinOS memd reservation M2: malformed taskd request rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    seL4_SetMR(0, SELINOS_TASKD_RES_M1_MEM_RESERVED);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_RES_M1_REPLY_WORDS));
    seL4_DebugPutString("SeLinOS memd reservation M2: status-only slot 1 reservation returned.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
