// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#ifndef SELINOS_SERVICE_NAME
#define SELINOS_SERVICE_NAME "unnamed"
#endif

int main(void)
{
    seL4_DebugPutString("SeLinOS M0 service online: " SELINOS_SERVICE_NAME "\n");

    /*
     * This M0 service deliberately owns no device or management capability.
     * It proves that the root task can construct and resume an isolated seL4
     * process. Its IPC implementation is introduced in the next iteration.
     */
    for (;;) {
        (void)seL4_Yield();
    }
}
