// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_taskd_reservation_m1_protocol.h"

static bool receive_exact_client_request(seL4_Word *badge)
{
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TASKD_RES_M1_CLIENT_ENDPOINT_SLOT, badge);

    return seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) == SELINOS_TASKD_RES_M1_REQUEST_WORDS &&
           seL4_GetMR(0) == SELINOS_TASKD_RES_M1_RESERVE_BUNDLES &&
           seL4_GetMR(1) == SELINOS_TASKD_RES_M1_SLOT;
}

static bool call_exact_downstream(seL4_CPtr endpoint, seL4_Word request,
                                  seL4_Word expected_reply)
{
    seL4_SetMR(0, request);
    seL4_SetMR(1, SELINOS_TASKD_RES_M1_SLOT);
    seL4_MessageInfo_t reply = seL4_Call(
        endpoint, seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_RES_M1_REQUEST_WORDS));

    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) == SELINOS_TASKD_RES_M1_REPLY_WORDS &&
           seL4_GetMR(0) == expected_reply;
}

int main(void)
{
    seL4_Word badge = 0u;

    if (!receive_exact_client_request(&badge)) {
        seL4_DebugPutString("SeLinOS taskd reservation M1: malformed combined request rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd reservation M1: exact combined slot 1 request accepted.\n");

    if (!call_exact_downstream(SELINOS_TASKD_RES_M1_OBJECT_ENDPOINT_SLOT,
                               SELINOS_TASKD_RES_M1_OBJECT_RESERVE,
                               SELINOS_TASKD_RES_M1_OBJECT_RESERVED)) {
        seL4_DebugPutString("SeLinOS taskd reservation M1: object reservation rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd reservation M1: objectd status-only reservation validated.\n");

    if (!call_exact_downstream(SELINOS_TASKD_RES_M1_MEM_ENDPOINT_SLOT,
                               SELINOS_TASKD_RES_M1_MEM_RESERVE,
                               SELINOS_TASKD_RES_M1_MEM_RESERVED)) {
        seL4_DebugPutString("SeLinOS taskd reservation M1: mapping reservation rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd reservation M1: memd status-only reservation validated.\n");

    seL4_Signal(SELINOS_TASKD_RES_M1_SUCCESS_NOTIFY_SLOT);
    seL4_DebugPutString("SeLinOS taskd reservation M1: both status-only reservations pending; no task created.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
