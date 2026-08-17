// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>
#include "selinos_taskd_dynamic_alloc_m0_protocol.h"

static void reply(seL4_Word status, seL4_Word generation)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_TASKD_DYN_M0_SLOT);
    seL4_SetMR(2, generation);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_TASKD_DYN_M0_REPLY_WORDS));
}
int main(void)
{
    bool reserved = false;
    seL4_Word generation = 0u;
    for (;;) {
        seL4_Word badge = 0u;
        seL4_MessageInfo_t m = seL4_Recv(SELINOS_TASKD_DYN_M0_SERVER_ENDPOINT_SLOT, &badge);
        if (seL4_MessageInfo_get_label(m) != 0u ||
            seL4_MessageInfo_get_length(m) != SELINOS_TASKD_DYN_M0_REQUEST_WORDS ||
            seL4_GetMR(1) != SELINOS_TASKD_DYN_M0_SLOT) {
            reply(SELINOS_TASKD_DYN_M0_EBUSY, generation);
            continue;
        }
        if (seL4_GetMR(0) == SELINOS_TASKD_DYN_M0_RESERVE) {
            if (reserved) { reply(SELINOS_TASKD_DYN_M0_EBUSY, generation); continue; }
            reserved = true; generation++; reply(SELINOS_TASKD_DYN_M0_RESERVED, generation); continue;
        }
        if (seL4_GetMR(0) == SELINOS_TASKD_DYN_M0_RELEASE && reserved) {
            reserved = false; reply(SELINOS_TASKD_DYN_M0_RELEASED, generation); continue;
        }
        reply(SELINOS_TASKD_DYN_M0_EBUSY, generation);
    }
}
