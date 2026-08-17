// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <selinos-root/gen_config.h>

#include "selinos_bootstrap.h"

static void debug_puts(const char *text)
{
    /* libsel4's debug ABI accepts a mutable pointer but does not modify it. */
    seL4_DebugPutString((char *)text);
}

static void selinos_exit(int code)
{
    (void)code;
    debug_puts("SeLinOS: root task attempted to exit; suspending.\n");
    (void)seL4_TCB_Suspend(seL4_CapInitThreadTCB);
    for (;;) {
        (void)seL4_Yield();
    }
}

int main(void)
{
    sel4runtime_set_exit(selinos_exit);

#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "selinos-rootd");
#endif

    debug_puts("SeLinOS M0: seL4 root task started.\n");

    if (!selinos_bootstrap_prepare()) {
        debug_puts("SeLinOS M0: bootstrap policy validation FAILED.\n");
        selinos_exit(1);
    }

    debug_puts("SeLinOS M0: bootstrap capabilities present.\n");
    debug_puts("SeLinOS M0: capability policy validation passed.\n");

    if (!selinos_domain_manager_start()) {
        debug_puts("SeLinOS M0: isolated domain bootstrap FAILED.\n");
        selinos_exit(2);
    }

    if (seL4_TCB_SetPriority(seL4_CapInitThreadTCB, seL4_CapInitThreadTCB, 0) != seL4_NoError) {
        debug_puts("SeLinOS M0: unable to lower root task priority.\n");
        selinos_exit(3);
    }

#if CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
    debug_puts("SeLinOS root construction M1: root entering dedicated blocking construction loop.\n");
    selinos_root_construct_m1_dispatch_loop();
#else
    if (!selinos_root_dispatch_m0_once()) {
        debug_puts("SeLinOS M0: root dispatch M0 transaction FAILED.\n");
        selinos_exit(4);
    }

    debug_puts("SeLinOS M0: root task entering idle/yield loop.\n");
#endif

    /*
     * No Linux code is present. No authority is delegated before objectd,
     * taskd, and memd are implemented. Yield instead of spinning at priority.
     */
    for (;;) {
        (void)seL4_Yield();
    }
}
