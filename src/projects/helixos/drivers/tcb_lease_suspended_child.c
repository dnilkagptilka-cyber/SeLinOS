// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

int main(void)
{
    /* This image is intentionally never spawned in Phase 32 M1. */
    seL4_DebugPutString("SeLinOS TCB lease M1 child: unexpected execution.\n");
    for (;;) { (void)seL4_Yield(); }
}
