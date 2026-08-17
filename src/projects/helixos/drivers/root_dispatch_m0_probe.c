// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_root_dispatch_m0_protocol.h"

int main(void)
{
    seL4_SetMR(0, SELINOS_ROOT_DISPATCH_M0_REQUEST);
    seL4_SetMR(1, SELINOS_ROOT_DISPATCH_M0_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_DISPATCH_M0_GENERATION);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_ROOT_DISPATCH_M0_PROBE_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_ROOT_DISPATCH_M0_WORDS));
    if (seL4_MessageInfo_get_label(reply) != 0u ||
        seL4_MessageInfo_get_length(reply) != SELINOS_ROOT_DISPATCH_M0_WORDS ||
        seL4_MessageInfo_get_extraCaps(reply) != 0u ||
        seL4_GetMR(0) != SELINOS_ROOT_DISPATCH_M0_READY ||
        seL4_GetMR(1) != SELINOS_ROOT_DISPATCH_M0_SLOT ||
        seL4_GetMR(2) != SELINOS_ROOT_DISPATCH_M0_GENERATION) {
        seL4_DebugPutString("SeLinOS root dispatch M0 probe: fixed reply rejected.\n");
        for (;;) { (void)seL4_Yield(); }
    }
    seL4_DebugPutString("SeLinOS root dispatch M0 probe: fixed root status transaction passed.\n");
    for (;;) { (void)seL4_Yield(); }
}
