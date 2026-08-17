// SPDX-License-Identifier: MIT
#ifndef SELINOS_KABI_EXPORTS_H
#define SELINOS_KABI_EXPORTS_H

#include "selinos_kabi_module.h"

struct selinos_kabi_export {
    const char *name;
    selinos_u32 crc;
    selinos_u64 runtime_address;
};

struct selinos_kabi_export_table {
    const struct selinos_kabi_export *entries;
    selinos_size_t count;
};

/* Resolver suitable for selinos_kabi_relocate_module. It exposes only entries
 * explicitly present in the provided version-pinned export table and refuses
 * null addresses, empty names and all non-listed imports. CRC validation of
 * actual module __versions records remains a separate KABI verification stage. */
int selinos_kabi_resolve_export(const char *name, selinos_u64 *runtime_address,
                                void *context);
int selinos_kabi_resolve_export_crc(const char *name, selinos_u32 *expected_crc,
                                    void *context);

#endif
