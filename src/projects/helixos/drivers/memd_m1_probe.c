// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_memd_m1_protocol.h"

static bool call_reserve_expect(seL4_Word expected_status)
{
    seL4_SetMR(0, SELINOS_MEMD_M1_RESERVE_FIXED_PLAN);
    seL4_SetMR(1, SELINOS_MEMD_M1_SLOT);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_MEMD_M1_PROBE_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_MEMD_M1_REQUEST_WORDS));

    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) == SELINOS_MEMD_M1_REPLY_WORDS &&
           seL4_GetMR(0) == expected_status;
}

int main(void)
{
    if (!call_reserve_expect(SELINOS_MEMD_M1_RESERVED) ||
        !call_reserve_expect(SELINOS_MEMD_M1_EBUSY)) {
        seL4_DebugPutString("SeLinOS memd M1 probe: fixed-plan reservation sequence rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    seL4_DebugPutString("SeLinOS memd M1 probe: one-slot fixed-plan reserve then EBUSY passed.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
