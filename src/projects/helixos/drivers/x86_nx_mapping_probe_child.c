// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_x86_nx_probe_protocol.h"

int main(void)
{
    void (*const target)(void) = (void (*)(void))SELINOS_X86_NX_PROBE_VADDR;

    seL4_DebugPutString("SeLinOS NX probe child: attempting one fixed indirect call.\n");
    target();

    /* Reaching this point proves only the paired executable control mapping. */
    seL4_SetMR(0, SELINOS_X86_NX_PROBE_SUCCESS_MAGIC);
    seL4_Send(SELINOS_X86_NX_PROBE_SUCCESS_ENDPOINT_SLOT,
              seL4_MessageInfo_new(0u, 0u, 0u,
                                   SELINOS_X86_NX_PROBE_SUCCESS_WORDS));
    seL4_DebugPutString("SeLinOS NX probe child: executable control returned once.\n");

    for (;;) {
        seL4_Yield();
    }
}
