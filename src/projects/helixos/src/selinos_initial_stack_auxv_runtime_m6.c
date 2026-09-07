// SPDX-License-Identifier: MIT
#include "selinos_initial_stack_auxv_runtime_m6.h"
#include "selinos_initial_stack_argv.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sel4/sel4.h>

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

static void debug_put_word(seL4_Word value)
{
    seL4_DebugPutString("0x");
    static const char digits[] = "0123456789abcdef";
    char output[sizeof(seL4_Word) * 2u + 1u];
    size_t index = sizeof(output) - 1u;
    output[index] = '\0';
    do {
        output[--index] = digits[value & 0xfu];
        value >>= 4u;
    } while (value != 0u && index != 0u);
    seL4_DebugPutString(output + index);
}

struct auxv_runtime_reader {
    const uint8_t *bytes;
    uintptr_t base;
    size_t length;
};

static bool read_authorized_byte(void *context, uintptr_t address, uint8_t *value)
{
    struct auxv_runtime_reader *reader = context;
    uintptr_t offset;
    if (reader == NULL || value == NULL || reader->bytes == NULL ||
        address < reader->base) {
        return false;
    }
    offset = address - reader->base;
    if (offset >= reader->length) {
        return false;
    }
    *value = reader->bytes[(size_t)offset];
    return true;
}

bool selinos_initial_stack_auxv_runtime_m6_witness(void)
{
    /* A bounded, local fixture models the bytes authorized by a stack mapping. */
    static const uintptr_t auxv[] = {
        3u, 0x00400040u, /* AT_PHDR */
        9u, 0x00401000u, /* AT_ENTRY */
        0u, 0u           /* AT_NULL */
    };
    struct auxv_runtime_reader reader = {
        .bytes = (const uint8_t *)auxv,
        .base = (uintptr_t)auxv,
        .length = sizeof(auxv)
    };
    struct selinos_initial_stack_policy policy = {
        .stack_base = (uintptr_t)auxv,
        .stack_bytes = sizeof(auxv),
        .max_argc = 1u,
        .max_envc = 1u,
        .max_auxv = 4u,
        .max_string_bytes = 1u,
        .page_bytes = 4096u,
        .allow_cross_page_strings = false
    };
    struct selinos_initial_stack_auxv_result result = {0};
    enum selinos_initial_stack_argv_status status;

    debug_puts("SeLinOS Phase 98 M6: auxv runtime witness begin.\n");
    status = selinos_initial_stack_auxv_parse(
        (uintptr_t)auxv, sizeof(auxv), &policy, read_authorized_byte,
        &reader, &result);
    if (status != SELINOS_INITIAL_STACK_ARGV_AUXV_VALIDATED ||
        result.entries_checked != 3u || !result.has_type ||
        result.last_type != 0u || result.last_value != 0u) {
        debug_puts("SeLinOS Phase 98 M6: auxv runtime witness FAILED. status=");
        debug_put_word((seL4_Word)status);
        debug_puts(" entries=");
        debug_put_word((seL4_Word)result.entries_checked);
        debug_puts(" last_type=");
        debug_put_word((seL4_Word)result.last_type);
        debug_puts(" last_value=");
        debug_put_word((seL4_Word)result.last_value);
        debug_puts("\n");
        return false;
    }
    debug_puts("SeLinOS Phase 98 M6: auxv runtime witness validated. entries=");
    debug_put_word((seL4_Word)result.entries_checked);
    debug_puts(" at_null_type=");
    debug_put_word((seL4_Word)result.last_type);
    debug_puts(" at_null_value=");
    debug_put_word((seL4_Word)result.last_value);
    debug_puts("\n");
    return true;
}
