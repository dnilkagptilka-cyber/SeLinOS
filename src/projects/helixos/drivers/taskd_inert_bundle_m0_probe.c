// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_taskd_inert_bundle_m0_protocol.h"

static bool call(seL4_Word expected_status)
{
    seL4_SetMR(0, SELINOS_TASKD_INERT_BUNDLE_M0_OWNERSHIP_QUERY);
    seL4_SetMR(1, SELINOS_TASKD_INERT_BUNDLE_M0_PLAN_SLOT);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_TASKD_INERT_BUNDLE_M0_PROBE_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u,
                             SELINOS_TASKD_INERT_BUNDLE_M0_REQUEST_WORDS));
    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) ==
               SELINOS_TASKD_INERT_BUNDLE_M0_REPLY_WORDS &&
           seL4_GetMR(0) == expected_status &&
           seL4_GetMR(1) == SELINOS_TASKD_INERT_BUNDLE_M0_PLAN_SLOT &&
           seL4_GetMR(2) == SELINOS_TASKD_INERT_BUNDLE_M0_OWNED_GENERATION;
}

int main(void)
{
    if (!call(SELINOS_TASKD_INERT_BUNDLE_M0_OWNED) ||
        !call(SELINOS_TASKD_INERT_BUNDLE_M0_REJECTED)) {
        seL4_DebugPutString("SeLinOS taskd inert bundle M0: ownership witness failed.\n");
        for (;;) { (void)seL4_Yield(); }
    }
    seL4_Signal(SELINOS_TASKD_INERT_BUNDLE_M0_PROBE_SUCCESS_NOTIFY_SLOT);
    seL4_DebugPutString("SeLinOS taskd inert bundle M0: rollback then bundle ownership query passed; no task constructed.\n");
    for (;;) { (void)seL4_Yield(); }
}
