// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_taskd_m1_protocol.h"

static bool receive_exact_client_start(seL4_Word *badge)
{
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TASKD_M1_CLIENT_ENDPOINT_SLOT, badge);

    return seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) == SELINOS_TASKD_M1_CLIENT_REQUEST_WORDS &&
           seL4_GetMR(0) == SELINOS_TASKD_M1_START &&
           seL4_GetMR(1) == SELINOS_TASKD_M1_CHILD_ID;
}

static bool receive_exact_child_completion(seL4_Word expected_generation, seL4_Word *badge)
{
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_TASKD_M1_CHILD_COMPLETION_SLOT, badge);

    return seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) == SELINOS_TASKD_M1_CHILD_COMPLETION_WORDS &&
           seL4_GetMR(0) == SELINOS_TASKD_M1_CHILD_DONE &&
           seL4_GetMR(1) == SELINOS_TASKD_M1_CHILD_ID &&
           seL4_GetMR(2) == expected_generation;
}

static void start_fixed_child(void)
{
    seL4_Signal(SELINOS_TASKD_M1_CHILD_START_SLOT);
}

int main(void)
{
    seL4_Word badge = 0u;

    if (!receive_exact_client_start(&badge)) {
        seL4_DebugPutString("SeLinOS taskd M1: malformed fixed-child request rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd M1: exact START(1) accepted.\n");

    start_fixed_child();
    if (!receive_exact_child_completion(SELINOS_TASKD_M1_FIRST_GENERATION, &badge)) {
        seL4_DebugPutString("SeLinOS taskd M1: first child completion rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd M1: child completion generation 1 validated.\n");

    if (seL4_TCB_Suspend(SELINOS_TASKD_M1_CHILD_TCB_SLOT) != seL4_NoError ||
        seL4_TCB_Resume(SELINOS_TASKD_M1_CHILD_TCB_SLOT) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd M1: fixed child TCB suspend/resume failed.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd M1: fixed child TCB suspended and resumed.\n");

    start_fixed_child();
    if (!receive_exact_child_completion(SELINOS_TASKD_M1_SECOND_GENERATION, &badge)) {
        seL4_DebugPutString("SeLinOS taskd M1: second child completion rejected.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    seL4_DebugPutString("SeLinOS taskd M1: child completion generation 2 validated.\n");

    seL4_Signal(SELINOS_TASKD_M1_PROBE_SUCCESS_SLOT);
    seL4_DebugPutString("SeLinOS taskd M1: fixed child lifecycle completed without clone semantics.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
