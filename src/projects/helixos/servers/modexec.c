// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_kabi_exports.h"
#include "selinos_kabi_module.h"
#include "selinos_kabi_policy.h"

extern const unsigned char selinos_kabi_probe_6_18_44_ko[];
extern const unsigned long selinos_kabi_probe_6_18_44_ko_size;

static unsigned char module_memory[65536];

typedef int (*selinos_module_init_t)(void);
typedef void (*selinos_module_exit_t)(void);

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

/* These are deliberately the only executable imports admitted for the pinned
 * test fixture. They have no device, IPC, memory-management or capability
 * authority. */
/* The kernel-built fixture passes one format pointer and does not establish
 * host System V variadic vector-register metadata. */
__attribute__((noinline)) static int selinos_module_printk(const char *format)
{
    (void)format;
    debug_puts("SeLinOS modexec: pinned fixture invoked curated _printk stub.\n");
    return 0;
}

__attribute__((noinline)) static void selinos_module_return_thunk(void)
{
}

int main(void)
{
    static const selinos_u8 fixture_digest[SELINOS_KABI_SHA256_BYTES] = {
        0xa4U, 0xc3U, 0xa6U, 0x61U, 0x4eU, 0x39U, 0x19U, 0x1aU,
        0xf7U, 0x07U, 0x1bU, 0xa3U, 0x20U, 0xb2U, 0xaaU, 0xc4U,
        0xaeU, 0x30U, 0x3aU, 0x9aU, 0xc8U, 0x15U, 0xc2U, 0xcdU,
        0x4dU, 0x86U, 0xd2U, 0xe1U, 0x57U, 0x5aU, 0x72U, 0x3dU,
    };
    struct selinos_kabi_export exports[3];
    struct selinos_kabi_export_table export_table;
    struct selinos_kabi_loaded_module loaded;
    const selinos_u64 load_base = (selinos_u64)(unsigned long)module_memory;
    const selinos_u64 printk_address = (selinos_u64)(unsigned long)selinos_module_printk;
    const selinos_u64 thunk_address = (selinos_u64)(unsigned long)selinos_module_return_thunk;

    exports[0] = (struct selinos_kabi_export){"_printk", 0x122c3a7eU, printk_address};
    exports[1] = (struct selinos_kabi_export){"__x86_return_thunk", 0x5b8239caU, thunk_address};
    exports[2] = (struct selinos_kabi_export){"module_layout", 0x3cfc2cadU,
                                               load_base + 0x8200U};
    export_table.entries = exports;
    export_table.count = 3U;

    if (selinos_kabi_check_pinned_digest(selinos_kabi_probe_6_18_44_ko,
                                         selinos_kabi_probe_6_18_44_ko_size,
                                         fixture_digest) != SELINOS_KABI_POLICY_OK ||
        selinos_kabi_verify_module_versions(selinos_kabi_probe_6_18_44_ko,
                                            selinos_kabi_probe_6_18_44_ko_size,
                                            "6.18.44", selinos_kabi_resolve_export_crc,
                                            &export_table) != SELINOS_KABI_MODULE_OK ||
        selinos_kabi_relocate_module(selinos_kabi_probe_6_18_44_ko,
                                     selinos_kabi_probe_6_18_44_ko_size, "6.18.44",
                                     module_memory, sizeof(module_memory),
                                     selinos_kabi_resolve_export, &export_table,
                                     &loaded) != SELINOS_KABI_MODULE_OK ||
        loaded.init_module == 0U || loaded.cleanup_module == 0U) {
        debug_puts("SeLinOS modexec: pinned fixture admission or relocation rejected.\n");
        return 1;
    }

    debug_puts("SeLinOS modexec: isolated pinned fixture init_module entering.\n");
    if (((selinos_module_init_t)(unsigned long)loaded.init_module)() != 0) {
        debug_puts("SeLinOS modexec: init_module returned failure.\n");
        return 1;
    }
    ((selinos_module_exit_t)(unsigned long)loaded.cleanup_module)();
    debug_puts("SeLinOS modexec: init_module and cleanup_module completed.\n");
    debug_puts("SeLinOS modexec: no device capability granted; NX-enforced W^X remains unproven.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
