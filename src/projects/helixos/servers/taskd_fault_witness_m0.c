// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_taskd_fault_witness_m0_protocol.h"

static void reply(seL4_Word status)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_TASKD_FAULT_WITNESS_M0_PLAN_SLOT);
    seL4_SetMR(2, SELINOS_TASKD_FAULT_WITNESS_M0_OWNED_GENERATION);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,
                                    SELINOS_TASKD_FAULT_WITNESS_M0_REPLY_WORDS));
}

int main(void)
{
    bool queried = false;
    for (;;) {
        seL4_Word badge = 0u;
        seL4_MessageInfo_t message = seL4_Recv(
            SELINOS_TASKD_FAULT_WITNESS_M0_SERVER_ENDPOINT_SLOT, &badge);
        bool exact_query = seL4_MessageInfo_get_label(message) == 0u &&
                           seL4_MessageInfo_get_length(message) ==
                               SELINOS_TASKD_FAULT_WITNESS_M0_REQUEST_WORDS &&
                           seL4_GetMR(0) ==
                               SELINOS_TASKD_FAULT_WITNESS_M0_OWNERSHIP_QUERY &&
                           seL4_GetMR(1) ==
                               SELINOS_TASKD_FAULT_WITNESS_M0_PLAN_SLOT;
        /* Root observed one deliberately unreplied VM fault after exactly one
         * resume. This service only reports that bounded state; it never
         * resumes, replies to, maps for, or invokes the target. */
        if (!exact_query || queried) {
            reply(SELINOS_TASKD_FAULT_WITNESS_M0_REJECTED);
            continue;
        }
        queried = true;
        reply(SELINOS_TASKD_FAULT_WITNESS_M0_OWNED);
        seL4_DebugPutString("SeLinOS taskd fault witness M0: fault-blocked target ownership issued; target not resumed.\n");
    }
}
