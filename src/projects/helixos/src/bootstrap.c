// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_bootstrap.h"

bool selinos_bootstrap_prepare(void)
{
    /*
     * These constants are seeded only in the initial task's CSpace by seL4.
     * M0 intentionally does not copy or delegate them. A later objectd
     * implementation will consume untyped capabilities through a reviewed
     * allocator and hand out reduced capabilities to child domains.
     */
    const seL4_CPtr bootstrap_tcb = seL4_CapInitThreadTCB;
    const seL4_CPtr bootstrap_cspace = seL4_CapInitThreadCNode;
    const seL4_CPtr bootstrap_vspace = seL4_CapInitThreadVSpace;

    return bootstrap_tcb != seL4_CapNull &&
           bootstrap_cspace != seL4_CapNull &&
           bootstrap_vspace != seL4_CapNull &&
           selinos_capability_policy_selftest();
}
