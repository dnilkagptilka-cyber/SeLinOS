// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_taskd_populated_cspace_m0_protocol.h"

static void reply(seL4_Word status)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_TASKD_POPULATED_CSPACE_M0_PLAN_SLOT);
    seL4_SetMR(2, SELINOS_TASKD_POPULATED_CSPACE_M0_OWNED_GENERATION);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,
                                    SELINOS_TASKD_POPULATED_CSPACE_M0_REPLY_WORDS));
}

int main(void)
{
    bool queried = false;
    for (;;) {
        seL4_Word badge = 0u;
        seL4_MessageInfo_t message = seL4_Recv(
            SELINOS_TASKD_POPULATED_CSPACE_M0_SERVER_ENDPOINT_SLOT, &badge);
        bool exact_query = seL4_MessageInfo_get_label(message) == 0u &&
                           seL4_MessageInfo_get_length(message) ==
                               SELINOS_TASKD_POPULATED_CSPACE_M0_REQUEST_WORDS &&
                           seL4_GetMR(0) ==
                               SELINOS_TASKD_POPULATED_CSPACE_M0_OWNERSHIP_QUERY &&
                           seL4_GetMR(1) ==
                               SELINOS_TASKD_POPULATED_CSPACE_M0_PLAN_SLOT;
        /* Root populated target CNode slot 1, configured the target TCB, then
         * moved final caps here. No final resource or target notification is invoked. */
        if (!exact_query || queried) {
            reply(SELINOS_TASKD_POPULATED_CSPACE_M0_REJECTED);
            continue;
        }
        queried = true;
        reply(SELINOS_TASKD_POPULATED_CSPACE_M0_OWNED);
        seL4_DebugPutString("SeLinOS taskd populated CSpace M0: ownership witness issued; target resources not invoked.\n");
    }
}
