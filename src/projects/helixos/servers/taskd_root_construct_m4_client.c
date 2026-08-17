// SPDX-License-Identifier: MIT
#include <stdbool.h>

#include <sel4/sel4.h>

#include "selinos_root_construct_m1_protocol.h"
#include "selinos_root_construct_m2_protocol.h"
#include "selinos_root_construct_m3_protocol.h"
#include "selinos_root_construct_m4_protocol.h"

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

static bool call_root_construct_m2_lease(seL4_Word expected_status,
                                         seL4_Word expected_caps)
{
    seL4_SetCapReceivePath(SELINOS_ROOT_CONSTRUCT_M2_TASKD_CNODE_SLOT,
                           SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT,
                           seL4_WordBits);
    seL4_SetMR(0, SELINOS_ROOT_CONSTRUCT_M2_LEASE_TCB);
    seL4_SetMR(1, SELINOS_ROOT_CONSTRUCT_M2_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_CONSTRUCT_M2_GENERATION);
    seL4_MessageInfo_t reply = seL4_Call(
        SELINOS_ROOT_CONSTRUCT_M2_TASKD_REQUEST_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u, 0u, 0u,
                             SELINOS_ROOT_CONSTRUCT_M2_REQUEST_WORDS));
    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) == SELINOS_ROOT_CONSTRUCT_M2_REPLY_WORDS &&
           seL4_MessageInfo_get_extraCaps(reply) == expected_caps &&
           seL4_GetMR(0) == expected_status &&
           seL4_GetMR(1) == SELINOS_ROOT_CONSTRUCT_M2_SLOT &&
           seL4_GetMR(2) == SELINOS_ROOT_CONSTRUCT_M2_GENERATION;
}

static bool receive_exact_child_completion(void)
{
    seL4_Word badge = 0u;
    const seL4_MessageInfo_t message = seL4_Recv(
        SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT, &badge);
    return badge == 0u &&
           seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) ==
               SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS &&
           seL4_MessageInfo_get_extraCaps(message) == 0u &&
           seL4_GetMR(0) == SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE &&
           seL4_GetMR(1) == SELINOS_ROOT_CONSTRUCT_M4_SLOT &&
           seL4_GetMR(2) == SELINOS_ROOT_CONSTRUCT_M4_GENERATION;
}

int main(void)
{
    if (!call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED) ||
        !call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED) ||
        !call_root_construct_m2_lease(SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED,
                                      SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS) ||
        !call_root_construct_m2_lease(SELINOS_ROOT_CONSTRUCT_M2_REJECTED,
                                      SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS)) {
        seL4_DebugPutString("SeLinOS root construction M4 taskd client: M1/M2 provenance transaction rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    if (seL4_TCB_Resume(SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS root construction M4 taskd client: received-child TCB resume rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS root construction M4 taskd client: one received-child TCB resumed; waiting for completion.\n");
    if (!receive_exact_child_completion()) {
        seL4_DebugPutString("SeLinOS root construction M4 taskd client: child completion rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS root construction M4 taskd client: exact one child completion validated.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
