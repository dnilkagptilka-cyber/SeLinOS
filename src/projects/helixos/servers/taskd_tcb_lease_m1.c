// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_tcb_lease_m1_protocol.h"

#define SELINOS_TCB_LEASE_M1_TASKD_CNODE_SLOT 1u

static bool receive_exact_client_request(seL4_Word *badge)
{
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TCB_LEASE_M1_TASKD_CLIENT_ENDPOINT_SLOT,
                                           badge);
    return seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) == SELINOS_TCB_LEASE_M1_REQUEST_WORDS &&
           seL4_GetMR(0) == SELINOS_TCB_LEASE_M1_REQUEST &&
           seL4_GetMR(1) == SELINOS_TCB_LEASE_M1_SLOT;
}

int main(void)
{
    seL4_Word badge = 0u;

    if (!receive_exact_client_request(&badge)) {
        seL4_DebugPutString("SeLinOS taskd TCB lease M1: malformed client request rejected.\n");
        for (;;) { (void)seL4_Yield(); }
    }
    seL4_DebugPutString("SeLinOS taskd TCB lease M1: exact slot 1 request accepted.\n");

    seL4_SetCapReceivePath(SELINOS_TCB_LEASE_M1_TASKD_CNODE_SLOT,
                           SELINOS_TCB_LEASE_M1_TASKD_TCB_DEST_SLOT,
                           seL4_WordBits);
    seL4_SetMR(0, SELINOS_TCB_LEASE_M1_REQUEST);
    seL4_SetMR(1, SELINOS_TCB_LEASE_M1_SLOT);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_TCB_LEASE_M1_TASKD_OBJECT_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TCB_LEASE_M1_REQUEST_WORDS));

    if (seL4_MessageInfo_get_label(reply) != 0u ||
        seL4_MessageInfo_get_length(reply) != SELINOS_TCB_LEASE_M1_REPLY_WORDS ||
        seL4_MessageInfo_get_extraCaps(reply) != SELINOS_TCB_LEASE_M1_REPLY_CAPS ||
        seL4_GetMR(0) != SELINOS_TCB_LEASE_M1_GRANTED) {
        seL4_DebugPutString("SeLinOS taskd TCB lease M1: TCB cap reply rejected.\n");
        for (;;) { (void)seL4_Yield(); }
    }

    seL4_Signal(SELINOS_TCB_LEASE_M1_TASKD_SUCCESS_NOTIFY_SLOT);
    seL4_DebugPutString("SeLinOS taskd TCB lease M1: suspended child TCB cap received and not exercised.\n");
    for (;;) { (void)seL4_Yield(); }
}
