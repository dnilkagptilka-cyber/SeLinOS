// SPDX-License-Identifier: MIT
#include <stdbool.h>

#include <sel4/sel4.h>

#include "selinos_root_construct_m2.h"
#include "selinos_root_construct_m2_protocol.h"

/* Root-private provenance for exactly the M1-created suspended child.  No
 * allocator, VSpace, process configuration, start or cleanup operation exists
 * in this M2 translation unit. */
static seL4_CPtr root_construct_m2_child_tcb = seL4_CapNull;
static bool root_construct_m2_lease_granted;

static void reply_root_construct_m2(seL4_Word status, seL4_Word extra_caps)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_ROOT_CONSTRUCT_M2_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_CONSTRUCT_M2_GENERATION);
    if (extra_caps == SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS) {
        seL4_SetCap(0u, root_construct_m2_child_tcb);
    } else {
        seL4_SetCap(0u, seL4_CapNull);
    }
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, extra_caps,
                                    SELINOS_ROOT_CONSTRUCT_M2_REPLY_WORDS));
}

bool selinos_root_construct_m2_record_child_tcb(seL4_CPtr child_tcb)
{
    if (child_tcb == seL4_CapNull || root_construct_m2_child_tcb != seL4_CapNull ||
        root_construct_m2_lease_granted) {
        return false;
    }
    root_construct_m2_child_tcb = child_tcb;
    return true;
}

bool selinos_root_construct_m2_try_dispatch(seL4_Word badge,
                                            seL4_MessageInfo_t message)
{
    if (seL4_MessageInfo_get_length(message) == 0u ||
        seL4_GetMR(0) != SELINOS_ROOT_CONSTRUCT_M2_LEASE_TCB) {
        return false;
    }

    const bool exact_request =
        badge == 0u &&
        seL4_MessageInfo_get_label(message) == 0u &&
        seL4_MessageInfo_get_length(message) ==
            SELINOS_ROOT_CONSTRUCT_M2_REQUEST_WORDS &&
        seL4_MessageInfo_get_extraCaps(message) == 0u &&
        seL4_GetMR(1) == SELINOS_ROOT_CONSTRUCT_M2_SLOT &&
        seL4_GetMR(2) == SELINOS_ROOT_CONSTRUCT_M2_GENERATION;

    if (!exact_request || root_construct_m2_child_tcb == seL4_CapNull ||
        root_construct_m2_lease_granted) {
        reply_root_construct_m2(SELINOS_ROOT_CONSTRUCT_M2_REJECTED,
                                SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS);
        seL4_DebugPutString("SeLinOS root construction M2: malformed, unavailable or duplicate lease rejected; no cap delivered.\n");
        return true;
    }

    root_construct_m2_lease_granted = true;
    reply_root_construct_m2(SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED,
                            SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS);
    seL4_DebugPutString("SeLinOS root construction M2: one constructed-child TCB control cap delivered; child remains suspended.\n");
    return true;
}
