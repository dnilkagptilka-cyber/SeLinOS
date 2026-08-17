// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

/* Phase 35 M1 must configure and spawn this root-selected image suspended.
 * Any execution is evidence of an unintended start and is therefore explicit. */
int main(void)
{
    seL4_DebugPutString("SeLinOS root construction M1 inert child: unexpected execution.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
