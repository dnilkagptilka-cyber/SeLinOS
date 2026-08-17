// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_root_construct_m4_protocol.h"

int main(void)
{
    seL4_DebugPutString("SeLinOS root construction M4 completion child: executed after taskd resume.\n");
    seL4_SetMR(0, SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE);
    seL4_SetMR(1, SELINOS_ROOT_CONSTRUCT_M4_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_CONSTRUCT_M4_GENERATION);
    seL4_Send(SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u,
                                   SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS));
    seL4_DebugPutString("SeLinOS root construction M4 completion child: one exact completion sent.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
