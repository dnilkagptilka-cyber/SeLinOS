// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

int main(void)
{
    seL4_DebugPutString("SeLinOS root construction M3 witness child: executed once after taskd resume.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
