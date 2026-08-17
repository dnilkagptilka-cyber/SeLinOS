// SPDX-License-Identifier: MIT
#ifndef SELINOS_KABI_MODULE_H
#define SELINOS_KABI_MODULE_H

/* seL4 root-task builds use -nostdinc; define the required ABI-width types. */
typedef unsigned long selinos_size_t;
typedef unsigned char selinos_u8;
typedef unsigned int selinos_u32;
typedef unsigned long selinos_u64;

/*
 * This parser is intentionally filesystem-agnostic. A future selmod service
 * supplies a complete .ko byte buffer after policy and signature handling.
 */
#define SELINOS_KABI_MODULE_NAME_MAX 64U
#define SELINOS_KABI_VERMAGIC_MAX 128U

struct selinos_kabi_module_summary {
    char name[SELINOS_KABI_MODULE_NAME_MAX];
    char vermagic[SELINOS_KABI_VERMAGIC_MAX];
    selinos_u32 elf_machine;
    selinos_u32 relocation_sections;
    selinos_u32 undefined_symbols;
    selinos_u32 modinfo_entries;
    int has_license;
    int has_versions;
};

enum selinos_kabi_module_status {
    SELINOS_KABI_MODULE_OK = 0,
    SELINOS_KABI_MODULE_E_ARGUMENT = -1,
    SELINOS_KABI_MODULE_E_TRUNCATED = -2,
    SELINOS_KABI_MODULE_E_ELF = -3,
    SELINOS_KABI_MODULE_E_ARCH = -4,
    SELINOS_KABI_MODULE_E_TYPE = -5,
    SELINOS_KABI_MODULE_E_SECTIONS = -6,
    SELINOS_KABI_MODULE_E_VERMAGIC = -7,
    SELINOS_KABI_MODULE_E_METADATA = -8,
    SELINOS_KABI_MODULE_E_MEMORY = -9,
    SELINOS_KABI_MODULE_E_RELOCATION = -10,
    SELINOS_KABI_MODULE_E_SYMBOL = -11,
};

/*
 * Parses an ELF64/x86_64 ET_REL Linux module. It does not load or execute any
 * module code and it never grants device authority. The caller must separately
 * verify signatures, required symbol CRCs and requested capabilities.
 */
int selinos_kabi_parse_module(const selinos_u8 *image,
                              selinos_size_t image_size,
                              const char *required_vermagic_prefix,
                              struct selinos_kabi_module_summary *summary);

/* Relocation stage is deliberately separate from parse/CRC/signature policy.
 * `resolve` receives only undefined symbol names and must return a previously
 * authorised runtime address. The loader copies SHF_ALLOC sections into the
 * caller-owned buffer and applies only its declared x86_64 relocation subset.
 * It neither maps executable pages nor invokes init_module/cleanup_module. */
typedef int (*selinos_kabi_symbol_resolver_t)(const char *name,
                                              selinos_u64 *runtime_address,
                                              void *context);
typedef int (*selinos_kabi_crc_resolver_t)(const char *name,
                                           selinos_u32 *expected_crc,
                                           void *context);

/* Validates each Linux CONFIG_MODVERSIONS record against a caller-approved
 * version-pinned symbol/CRC resolver. No relocation or execution occurs. */
int selinos_kabi_verify_module_versions(const selinos_u8 *image,
                                        selinos_size_t image_size,
                                        const char *required_vermagic_prefix,
                                        selinos_kabi_crc_resolver_t resolve_crc,
                                        void *resolve_context);

struct selinos_kabi_loaded_module {
    selinos_u64 load_base;
    selinos_size_t image_size;
    selinos_u64 init_module;
    selinos_u64 cleanup_module;
    selinos_u32 relocated_sections;
    selinos_u32 applied_relocations;
};

int selinos_kabi_relocate_module(const selinos_u8 *image,
                                 selinos_size_t image_size,
                                 const char *required_vermagic_prefix,
                                 selinos_u8 *load_memory,
                                 selinos_size_t load_memory_size,
                                 selinos_kabi_symbol_resolver_t resolve,
                                 void *resolve_context,
                                 struct selinos_kabi_loaded_module *loaded);

#endif
