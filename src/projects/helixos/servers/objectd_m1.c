// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <sel4/sel4.h>

#include "selinos_objectd_m1_protocol.h"

static bool request_is_exact_reserve(seL4_MessageInfo_t message)
{
    return seL4_MessageInfo_get_label(message) == 0u &&
           seL4_MessageInfo_get_length(message) == SELINOS_OBJECTD_M1_REQUEST_WORDS &&
           seL4_GetMR(0) == SELINOS_OBJECTD_M1_RESERVE &&
           seL4_GetMR(1) == SELINOS_OBJECTD_M1_SLOT;
}

static void reply_status(seL4_Word status)
{
    seL4_SetMR(0, status);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_OBJECTD_M1_REPLY_WORDS));
}

int main(void)
{
    bool reserved = false;
    seL4_Word badge = 0u;

    for (;;) {
        seL4_MessageInfo_t message = seL4_Recv(SELINOS_OBJECTD_M1_ENDPOINT_SLOT, &badge);
        if (!request_is_exact_reserve(message)) {
            reply_status(SELINOS_OBJECTD_M1_EINVAL);
            seL4_DebugPutString("SeLinOS objectd M1: malformed reservation rejected.\n");
            continue;
        }
        if (!reserved) {
            reserved = true;
            reply_status(SELINOS_OBJECTD_M1_RESERVED);
            seL4_DebugPutString("SeLinOS objectd M1: fixed inventory slot 1 reserved without object allocation.\n");
            continue;
        }

        reply_status(SELINOS_OBJECTD_M1_EBUSY);
        seL4_DebugPutString("SeLinOS objectd M1: duplicate slot 1 reservation rejected with EBUSY.\n");
        seL4_DebugPutString("SeLinOS objectd M1: fixed inventory reservation-state proof passed.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
}
