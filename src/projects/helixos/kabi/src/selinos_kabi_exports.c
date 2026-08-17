// SPDX-License-Identifier: MIT
#include "selinos_kabi_exports.h"

static int names_equal(const char *left, const char *right)
{
    selinos_size_t index = 0U;

    if (left == (void *)0 || right == (void *)0) {
        return 0;
    }
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return 0;
        }
        ++index;
    }
    return left[index] == '\0' && right[index] == '\0';
}

int selinos_kabi_resolve_export(const char *name, selinos_u64 *runtime_address,
                                void *context)
{
    const struct selinos_kabi_export_table *table =
        (const struct selinos_kabi_export_table *)context;
    selinos_size_t index;

    if (name == (void *)0 || runtime_address == (void *)0 || table == (void *)0 ||
        table->entries == (void *)0 || table->count == 0U || name[0] == '\0') {
        return 0;
    }
    for (index = 0U; index < table->count; ++index) {
        const struct selinos_kabi_export *entry = &table->entries[index];
        if (entry->runtime_address != 0U && names_equal(name, entry->name)) {
            *runtime_address = entry->runtime_address;
            return 1;
        }
    }
    return 0;
}

int selinos_kabi_resolve_export_crc(const char *name, selinos_u32 *expected_crc,
                                    void *context)
{
    const struct selinos_kabi_export_table *table =
        (const struct selinos_kabi_export_table *)context;
    selinos_size_t index;

    if (name == (void *)0 || expected_crc == (void *)0 || table == (void *)0 ||
        table->entries == (void *)0 || table->count == 0U || name[0] == '\0') {
        return 0;
    }
    for (index = 0U; index < table->count; ++index) {
        const struct selinos_kabi_export *entry = &table->entries[index];
        if (entry->runtime_address != 0U && names_equal(name, entry->name)) {
            *expected_crc = entry->crc;
            return 1;
        }
    }
    return 0;
}
