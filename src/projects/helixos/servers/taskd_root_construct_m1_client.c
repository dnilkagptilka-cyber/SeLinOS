// SPDX-License-Identifier: MIT
#include <stdbool.h>

#include <sel4/sel4.h>

#include "selinos_root_construct_m1_protocol.h"

static bool call_root_construct_m1(seL4_Word expected_status)
{
    seL4_SetMR(0, SELINOS_ROOT_CONSTRUCT_M1_REQUEST);
    seL4_SetMR(1, SELINOS_ROOT_CONSTRUCT_M1_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_CONSTRUCT_M1_GENERATION);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_ROOT_CONSTRUCT_M1_TASKD_REQUEST_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u,
                             SELINOS_ROOT_CONSTRUCT_M1_REQUEST_WORDS));

    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) == SELINOS_ROOT_CONSTRUCT_M1_REPLY_WORDS &&
           seL4_MessageInfo_get_extraCaps(reply) == 0u &&
           seL4_GetMR(0) == expected_status &&
           seL4_GetMR(1) == SELINOS_ROOT_CONSTRUCT_M1_SLOT &&
           seL4_GetMR(2) == SELINOS_ROOT_CONSTRUCT_M1_GENERATION;
}

int main(void)
{
    if (!call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED)) {
        seL4_DebugPutString("SeLinOS root construction M1 taskd client: construction reply rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    if (!call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED)) {
        seL4_DebugPutString("SeLinOS root construction M1 taskd client: duplicate refusal rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    seL4_DebugPutString("SeLinOS root construction M1 taskd client: fixed construction and duplicate refusal passed; no child cap received.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
