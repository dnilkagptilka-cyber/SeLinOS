// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_taskd_m1_protocol.h"

static void send_completion(seL4_Word generation)
{
    seL4_SetMR(0, SELINOS_TASKD_M1_CHILD_DONE);
    seL4_SetMR(1, SELINOS_TASKD_M1_CHILD_ID);
    seL4_SetMR(2, generation);
    seL4_Send(SELINOS_TASKD_M1_CHILD_COMPLETION_SEND_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_M1_CHILD_COMPLETION_WORDS));
}

int main(void)
{
    seL4_Word badge = 0u;

    (void)seL4_Wait(SELINOS_TASKD_M1_CHILD_START_WAIT_SLOT, &badge);
    send_completion(SELINOS_TASKD_M1_FIRST_GENERATION);

    (void)seL4_Wait(SELINOS_TASKD_M1_CHILD_START_WAIT_SLOT, &badge);
    send_completion(SELINOS_TASKD_M1_SECOND_GENERATION);

    for (;;) {
        (void)seL4_Yield();
    }
}
