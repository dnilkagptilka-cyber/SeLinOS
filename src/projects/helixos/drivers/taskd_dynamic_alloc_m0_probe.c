// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>
#include "selinos_taskd_dynamic_alloc_m0_protocol.h"
static bool call(seL4_Word op, seL4_Word status, seL4_Word generation)
{
    seL4_SetMR(0, op); seL4_SetMR(1, SELINOS_TASKD_DYN_M0_SLOT);
    seL4_MessageInfo_t r = seL4_Call(SELINOS_TASKD_DYN_M0_PROBE_ENDPOINT_SLOT,
        seL4_MessageInfo_new(0u,0u,0u,SELINOS_TASKD_DYN_M0_REQUEST_WORDS));
    return seL4_MessageInfo_get_label(r)==0u && seL4_MessageInfo_get_length(r)==SELINOS_TASKD_DYN_M0_REPLY_WORDS &&
        seL4_GetMR(0)==status && seL4_GetMR(1)==SELINOS_TASKD_DYN_M0_SLOT && seL4_GetMR(2)==generation;
}
int main(void)
{
    if (!call(SELINOS_TASKD_DYN_M0_RESERVE,SELINOS_TASKD_DYN_M0_RESERVED,1u) ||
        !call(SELINOS_TASKD_DYN_M0_RESERVE,SELINOS_TASKD_DYN_M0_EBUSY,1u) ||
        !call(SELINOS_TASKD_DYN_M0_RELEASE,SELINOS_TASKD_DYN_M0_RELEASED,1u) ||
        !call(SELINOS_TASKD_DYN_M0_RESERVE,SELINOS_TASKD_DYN_M0_RESERVED,2u)) {
        seL4_DebugPutString("SeLinOS taskd dynamic M0: bounded allocation lifecycle failed.\n");
        for (;;) (void)seL4_Yield();
    }
    seL4_Signal(SELINOS_TASKD_DYN_M0_PROBE_SUCCESS_NOTIFY_SLOT);
    seL4_DebugPutString("SeLinOS taskd dynamic M0: reserve/reject/release/re-reserve passed; no task created.\n");
    for (;;) (void)seL4_Yield();
}
