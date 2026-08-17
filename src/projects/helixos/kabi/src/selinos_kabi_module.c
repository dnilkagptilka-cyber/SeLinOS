// SPDX-License-Identifier: MIT
#include "selinos_kabi_module.h"

/* ELF64 definitions kept local to avoid a host libc dependency in seL4 builds. */
typedef unsigned short selinos_u16;
typedef unsigned long selinos_u64;

#define SELINOS_NULL ((void *)0)
#define SELINOS_EI_CLASS 4U
#define SELINOS_EI_DATA 5U
#define SELINOS_EI_VERSION 6U
#define SELINOS_ELFCLASS64 2U
#define SELINOS_ELFDATA2LSB 1U
#define SELINOS_EV_CURRENT 1U
#define SELINOS_ET_REL 1U
#define SELINOS_EM_X86_64 62U
#define SELINOS_SHT_SYMTAB 2U
#define SELINOS_SHT_RELA 4U
#define SELINOS_SHN_UNDEF 0U

struct selinos_elf64_ehdr {
    selinos_u8 ident[16];
    selinos_u16 type;
    selinos_u16 machine;
    selinos_u32 version;
    selinos_u64 entry;
    selinos_u64 phoff;
    selinos_u64 shoff;
    selinos_u32 flags;
    selinos_u16 ehsize;
    selinos_u16 phentsize;
    selinos_u16 phnum;
    selinos_u16 shentsize;
    selinos_u16 shnum;
    selinos_u16 shstrndx;
};

struct selinos_elf64_shdr {
    selinos_u32 name;
    selinos_u32 type;
    selinos_u64 flags;
    selinos_u64 addr;
    selinos_u64 offset;
    selinos_u64 size;
    selinos_u32 link;
    selinos_u32 info;
    selinos_u64 addralign;
    selinos_u64 entsize;
};

struct selinos_elf64_sym {
    selinos_u32 name;
    selinos_u8 info;
    selinos_u8 other;
    selinos_u16 shndx;
    selinos_u64 value;
    selinos_u64 size;
};

static int range_ok(selinos_size_t image_size, selinos_u64 offset, selinos_u64 length)
{
    return offset <= image_size && length <= image_size - offset;
}

static void zero_bytes(void *target, selinos_size_t size)
{
    selinos_u8 *bytes = (selinos_u8 *)target;
    selinos_size_t index;

    for (index = 0; index < size; index++) {
        bytes[index] = 0;
    }
}

static int bytes_equal(const selinos_u8 *left, const selinos_u8 *right, selinos_size_t count)
{
    selinos_size_t index;

    for (index = 0; index < count; index++) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static int string_terminated(const char *text, selinos_size_t limit)
{
    selinos_size_t index;

    for (index = 0U; index < limit; ++index) {
        if (text[index] == '\0') {
            return 1;
        }
    }
    return 0;
}

static int name_equals(const char *candidate, selinos_size_t candidate_limit, const char *expected)
{
    selinos_size_t index = 0;

    while (expected[index] != '\0') {
        if (index >= candidate_limit || candidate[index] != expected[index]) {
            return 0;
        }
        index++;
    }
    return index < candidate_limit && candidate[index] == '\0';
}

static void copy_value(char *target, selinos_size_t target_size, const char *value, selinos_size_t value_size)
{
    selinos_size_t copied = 0;

    if (target_size == 0) {
        return;
    }
    while (copied + 1 < target_size && copied < value_size && value[copied] != '\0') {
        target[copied] = value[copied];
        copied++;
    }
    target[copied] = '\0';
}

static int begins_with(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (*text++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void parse_modinfo(const char *data, selinos_size_t data_size, struct selinos_kabi_module_summary *summary)
{
    selinos_size_t at = 0;

    while (at < data_size) {
        const char *entry = data + at;
        selinos_size_t entry_size = 0;

        while (at + entry_size < data_size && entry[entry_size] != '\0') {
            entry_size++;
        }
        if (entry_size == 0) {
            at++;
            continue;
        }
        summary->modinfo_entries++;
        if (entry_size > 5 && bytes_equal((const selinos_u8 *)entry, (const selinos_u8 *)"name=", 5)) {
            copy_value(summary->name, sizeof(summary->name), entry + 5, entry_size - 5);
        } else if (entry_size > 9 && bytes_equal((const selinos_u8 *)entry, (const selinos_u8 *)"vermagic=", 9)) {
            copy_value(summary->vermagic, sizeof(summary->vermagic), entry + 9, entry_size - 9);
        } else if (entry_size > 8 && bytes_equal((const selinos_u8 *)entry, (const selinos_u8 *)"license=", 8)) {
            summary->has_license = 1;
        }
        at += entry_size + 1;
    }
}

int selinos_kabi_parse_module(const selinos_u8 *image,
                              selinos_size_t image_size,
                              const char *required_vermagic_prefix,
                              struct selinos_kabi_module_summary *summary)
{
    const struct selinos_elf64_ehdr *header;
    const struct selinos_elf64_shdr *sections;
    const char *section_names;
    selinos_size_t section_names_size;
    selinos_size_t table_size;
    selinos_u16 index;

    if (image == SELINOS_NULL || summary == SELINOS_NULL || required_vermagic_prefix == SELINOS_NULL) {
        return SELINOS_KABI_MODULE_E_ARGUMENT;
    }
    zero_bytes(summary, sizeof(*summary));
    if (image_size < sizeof(struct selinos_elf64_ehdr)) {
        return SELINOS_KABI_MODULE_E_TRUNCATED;
    }

    header = (const struct selinos_elf64_ehdr *)image;
    if (!bytes_equal(header->ident, (const selinos_u8 *)"\177ELF", 4) ||
        header->ident[SELINOS_EI_CLASS] != SELINOS_ELFCLASS64 ||
        header->ident[SELINOS_EI_DATA] != SELINOS_ELFDATA2LSB ||
        header->ident[SELINOS_EI_VERSION] != SELINOS_EV_CURRENT) {
        return SELINOS_KABI_MODULE_E_ELF;
    }
    if (header->machine != SELINOS_EM_X86_64) {
        return SELINOS_KABI_MODULE_E_ARCH;
    }
    if (header->type != SELINOS_ET_REL) {
        return SELINOS_KABI_MODULE_E_TYPE;
    }
    if (header->shentsize != sizeof(struct selinos_elf64_shdr) || header->shnum == 0 ||
        header->shstrndx == SELINOS_SHN_UNDEF || header->shstrndx >= header->shnum) {
        return SELINOS_KABI_MODULE_E_SECTIONS;
    }
    table_size = (selinos_size_t)header->shentsize * header->shnum;
    if (!range_ok(image_size, header->shoff, table_size)) {
        return SELINOS_KABI_MODULE_E_TRUNCATED;
    }
    sections = (const struct selinos_elf64_shdr *)(image + header->shoff);
    if (!range_ok(image_size, sections[header->shstrndx].offset, sections[header->shstrndx].size)) {
        return SELINOS_KABI_MODULE_E_TRUNCATED;
    }
    section_names = (const char *)(image + sections[header->shstrndx].offset);
    section_names_size = (selinos_size_t)sections[header->shstrndx].size;

    for (index = 0; index < header->shnum; index++) {
        const struct selinos_elf64_shdr *section = &sections[index];
        const char *section_name;
        selinos_size_t section_name_limit;

        if (section->name >= section_names_size || !range_ok(image_size, section->offset, section->size)) {
            return SELINOS_KABI_MODULE_E_SECTIONS;
        }
        section_name = section_names + section->name;
        section_name_limit = section_names_size - section->name;
        if (section->type == SELINOS_SHT_RELA) {
            summary->relocation_sections++;
        }
        if (section->type == SELINOS_SHT_SYMTAB) {
            const struct selinos_elf64_sym *symbols;
            selinos_size_t symbol_count;
            selinos_size_t symbol_index;

            if (section->entsize != sizeof(struct selinos_elf64_sym) || section->size % section->entsize != 0) {
                return SELINOS_KABI_MODULE_E_SECTIONS;
            }
            symbols = (const struct selinos_elf64_sym *)(image + section->offset);
            symbol_count = (selinos_size_t)(section->size / section->entsize);
            for (symbol_index = 0; symbol_index < symbol_count; symbol_index++) {
                if (symbols[symbol_index].shndx == SELINOS_SHN_UNDEF && symbols[symbol_index].name != 0) {
                    summary->undefined_symbols++;
                }
            }
        }
        if (name_equals(section_name, section_name_limit, ".modinfo")) {
            parse_modinfo((const char *)(image + section->offset), (selinos_size_t)section->size, summary);
        }
        if (name_equals(section_name, section_name_limit, "__versions")) {
            summary->has_versions = 1;
        }
    }

    if (summary->name[0] == '\0' || summary->vermagic[0] == '\0' || !summary->has_license) {
        return SELINOS_KABI_MODULE_E_METADATA;
    }
    if (!begins_with(summary->vermagic, required_vermagic_prefix)) {
        return SELINOS_KABI_MODULE_E_VERMAGIC;
    }
    summary->elf_machine = header->machine;
    return SELINOS_KABI_MODULE_OK;
}

/* Narrow non-executing ET_REL relocation stage for the pinned x86_64 profile. */
typedef long selinos_s64;

#define SELINOS_SHT_NOBITS 8U
#define SELINOS_SHF_ALLOC 0x2UL
#define SELINOS_R_X86_64_64 1U
#define SELINOS_R_X86_64_PC32 2U
#define SELINOS_R_X86_64_PLT32 4U
#define SELINOS_R_X86_64_32S 11U
#define SELINOS_MODULE_MAX_SECTIONS 256U

struct selinos_elf64_rela {
    selinos_u64 offset;
    selinos_u64 info;
    selinos_s64 addend;
};

static void copy_bytes(selinos_u8 *target, const selinos_u8 *source, selinos_size_t size)
{
    selinos_size_t index;

    for (index = 0; index < size; ++index) {
        target[index] = source[index];
    }
}

static int is_power_of_two(selinos_u64 value)
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

static int align_offset(selinos_size_t value, selinos_u64 alignment,
                        selinos_size_t limit, selinos_size_t *result)
{
    selinos_size_t aligned;

    if (alignment == 0U) {
        alignment = 1U;
    }
    if (!is_power_of_two(alignment) || alignment > (selinos_u64)limit) {
        return 0;
    }
    aligned = (value + (selinos_size_t)alignment - 1U) &
              ~((selinos_size_t)alignment - 1U);
    if (aligned < value || aligned > limit) {
        return 0;
    }
    *result = aligned;
    return 1;
}

static void write_u32(selinos_u8 *target, selinos_u32 value)
{
    target[0] = (selinos_u8)value;
    target[1] = (selinos_u8)(value >> 8U);
    target[2] = (selinos_u8)(value >> 16U);
    target[3] = (selinos_u8)(value >> 24U);
}

static void write_u64(selinos_u8 *target, selinos_u64 value)
{
    unsigned int index;

    for (index = 0U; index < 8U; ++index) {
        target[index] = (selinos_u8)(value >> (8U * index));
    }
}

static int signed_32_ok(selinos_s64 value)
{
    return value >= (selinos_s64)-2147483648LL && value <= (selinos_s64)2147483647LL;
}

static int symbol_runtime_address(const struct selinos_elf64_sym *symbol,
                                  const char *string_table,
                                  selinos_size_t string_table_size,
                                  const selinos_u64 *section_addresses,
                                  selinos_u16 section_count,
                                  selinos_kabi_symbol_resolver_t resolve,
                                  void *resolve_context,
                                  selinos_u64 *address)
{
    const char *name;
    selinos_size_t name_limit;

    if (symbol->shndx != SELINOS_SHN_UNDEF) {
        if (symbol->shndx >= section_count || section_addresses[symbol->shndx] == 0U) {
            return 0;
        }
        *address = section_addresses[symbol->shndx] + symbol->value;
        return 1;
    }
    if (resolve == SELINOS_NULL || symbol->name >= string_table_size) {
        return 0;
    }
    name = string_table + symbol->name;
    name_limit = string_table_size - symbol->name;
    if (!string_terminated(name, name_limit) || name[0] == '\0' ||
        !resolve(name, address, resolve_context) || *address == 0U) {
        return 0;
    }
    (void)name_limit;
    return 1;
}

int selinos_kabi_relocate_module(const selinos_u8 *image,
                                 selinos_size_t image_size,
                                 const char *required_vermagic_prefix,
                                 selinos_u8 *load_memory,
                                 selinos_size_t load_memory_size,
                                 selinos_kabi_symbol_resolver_t resolve,
                                 void *resolve_context,
                                 struct selinos_kabi_loaded_module *loaded)
{
    struct selinos_kabi_module_summary summary;
    const struct selinos_elf64_ehdr *header;
    const struct selinos_elf64_shdr *sections;
    const struct selinos_elf64_shdr *symbol_section = SELINOS_NULL;
    const struct selinos_elf64_sym *symbols = SELINOS_NULL;
    const char *symbol_names = SELINOS_NULL;
    selinos_size_t symbol_names_size = 0U;
    selinos_size_t symbol_count = 0U;
    selinos_u64 section_addresses[SELINOS_MODULE_MAX_SECTIONS];
    selinos_size_t used = 0U;
    selinos_u16 index;

    if (loaded == SELINOS_NULL || load_memory == SELINOS_NULL || load_memory_size == 0U) {
        return SELINOS_KABI_MODULE_E_ARGUMENT;
    }
    zero_bytes(loaded, sizeof(*loaded));
    if (selinos_kabi_parse_module(image, image_size, required_vermagic_prefix, &summary) !=
        SELINOS_KABI_MODULE_OK) {
        return SELINOS_KABI_MODULE_E_ELF;
    }
    header = (const struct selinos_elf64_ehdr *)image;
    if (header->shnum > SELINOS_MODULE_MAX_SECTIONS) {
        return SELINOS_KABI_MODULE_E_SECTIONS;
    }
    sections = (const struct selinos_elf64_shdr *)(image + header->shoff);
    zero_bytes(section_addresses, sizeof(section_addresses));

    /* Layout and copy every allocatable section inside caller-owned bounded
     * memory. No virtual-memory mapping or execute permission is created. */
    for (index = 0U; index < header->shnum; ++index) {
        const struct selinos_elf64_shdr *section = &sections[index];
        selinos_size_t aligned;

        if ((section->flags & SELINOS_SHF_ALLOC) == 0U || section->size == 0U) {
            continue;
        }
        if (!align_offset(used, section->addralign, load_memory_size, &aligned) ||
            section->size > load_memory_size - aligned) {
            return SELINOS_KABI_MODULE_E_MEMORY;
        }
        if (section->type != SELINOS_SHT_NOBITS &&
            !range_ok(image_size, section->offset, section->size)) {
            return SELINOS_KABI_MODULE_E_SECTIONS;
        }
        section_addresses[index] = (selinos_u64)(unsigned long)(load_memory + aligned);
        if (section->type == SELINOS_SHT_NOBITS) {
            zero_bytes(load_memory + aligned, (selinos_size_t)section->size);
        } else {
            copy_bytes(load_memory + aligned, image + section->offset,
                       (selinos_size_t)section->size);
        }
        used = aligned + (selinos_size_t)section->size;
        loaded->relocated_sections++;
    }

    for (index = 0U; index < header->shnum; ++index) {
        const struct selinos_elf64_shdr *section = &sections[index];
        if (section->type != SELINOS_SHT_SYMTAB) {
            continue;
        }
        if (symbol_section != SELINOS_NULL || section->entsize != sizeof(struct selinos_elf64_sym) ||
            section->size % section->entsize != 0U || section->link >= header->shnum ||
            !range_ok(image_size, section->offset, section->size)) {
            return SELINOS_KABI_MODULE_E_SECTIONS;
        }
        symbol_section = section;
        symbols = (const struct selinos_elf64_sym *)(image + section->offset);
        symbol_count = (selinos_size_t)(section->size / section->entsize);
        if (!range_ok(image_size, sections[section->link].offset, sections[section->link].size)) {
            return SELINOS_KABI_MODULE_E_SECTIONS;
        }
        symbol_names = (const char *)(image + sections[section->link].offset);
        symbol_names_size = (selinos_size_t)sections[section->link].size;
    }
    if (symbol_section == SELINOS_NULL || symbols == SELINOS_NULL || symbol_names == SELINOS_NULL) {
        return SELINOS_KABI_MODULE_E_SECTIONS;
    }

    for (index = 0U; index < header->shnum; ++index) {
        const struct selinos_elf64_shdr *relocation_section = &sections[index];
        const struct selinos_elf64_shdr *target;
        const struct selinos_elf64_rela *relocations;
        selinos_size_t relocation_count;
        selinos_size_t relocation_index;

        if (relocation_section->type != SELINOS_SHT_RELA) {
            continue;
        }
        if (relocation_section->info >= header->shnum ||
            relocation_section->link >= header->shnum ||
            relocation_section->link != (selinos_u32)(symbol_section - sections) ||
            relocation_section->entsize != sizeof(struct selinos_elf64_rela) ||
            relocation_section->size % relocation_section->entsize != 0U ||
            !range_ok(image_size, relocation_section->offset, relocation_section->size)) {
            return SELINOS_KABI_MODULE_E_RELOCATION;
        }
        target = &sections[relocation_section->info];
        if ((target->flags & SELINOS_SHF_ALLOC) == 0U || target->size == 0U) {
            continue;
        }
        relocations = (const struct selinos_elf64_rela *)(image + relocation_section->offset);
        relocation_count = (selinos_size_t)(relocation_section->size / relocation_section->entsize);
        for (relocation_index = 0U; relocation_index < relocation_count; ++relocation_index) {
            const struct selinos_elf64_rela *relocation = &relocations[relocation_index];
            const selinos_u32 type = (selinos_u32)relocation->info;
            const selinos_u32 symbol_index = (selinos_u32)(relocation->info >> 32U);
            const selinos_size_t width = type == SELINOS_R_X86_64_64 ? 8U : 4U;
            selinos_u64 symbol_address;
            selinos_u64 place;
            selinos_s64 value;
            selinos_u8 *target_address;

            if (symbol_index >= symbol_count || section_addresses[relocation_section->info] == 0U ||
                relocation->offset > target->size || width > target->size - relocation->offset ||
                !symbol_runtime_address(&symbols[symbol_index], symbol_names, symbol_names_size,
                                        section_addresses, header->shnum, resolve, resolve_context,
                                        &symbol_address)) {
                return SELINOS_KABI_MODULE_E_SYMBOL;
            }
            target_address = (selinos_u8 *)(unsigned long)section_addresses[relocation_section->info] +
                             (selinos_size_t)relocation->offset;
            place = (selinos_u64)(unsigned long)target_address;
            if (type == SELINOS_R_X86_64_64) {
                write_u64(target_address, symbol_address + (selinos_u64)relocation->addend);
            } else if (type == SELINOS_R_X86_64_PC32 || type == SELINOS_R_X86_64_PLT32) {
                value = (selinos_s64)symbol_address + relocation->addend - (selinos_s64)place;
                if (!signed_32_ok(value)) {
                    return SELINOS_KABI_MODULE_E_RELOCATION;
                }
                write_u32(target_address, (selinos_u32)value);
            } else if (type == SELINOS_R_X86_64_32S) {
                value = (selinos_s64)symbol_address + relocation->addend;
                if (!signed_32_ok(value)) {
                    return SELINOS_KABI_MODULE_E_RELOCATION;
                }
                write_u32(target_address, (selinos_u32)value);
            } else {
                return SELINOS_KABI_MODULE_E_RELOCATION;
            }
            loaded->applied_relocations++;
        }
    }

    for (selinos_size_t symbol_index = 0U; symbol_index < symbol_count; ++symbol_index) {
        const struct selinos_elf64_sym *symbol = &symbols[symbol_index];
        const char *name;
        selinos_size_t name_limit;
        selinos_u64 address;

        if (symbol->name >= symbol_names_size || symbol->shndx == SELINOS_SHN_UNDEF ||
            !symbol_runtime_address(symbol, symbol_names, symbol_names_size, section_addresses,
                                    header->shnum, resolve, resolve_context, &address)) {
            continue;
        }
        name = symbol_names + symbol->name;
        name_limit = symbol_names_size - symbol->name;
        if (name_equals(name, name_limit, "init_module")) {
            loaded->init_module = address;
        } else if (name_equals(name, name_limit, "cleanup_module")) {
            loaded->cleanup_module = address;
        }
    }
    if (loaded->init_module == 0U || loaded->cleanup_module == 0U) {
        return SELINOS_KABI_MODULE_E_SYMBOL;
    }
    loaded->load_base = (selinos_u64)(unsigned long)load_memory;
    loaded->image_size = used;
    return SELINOS_KABI_MODULE_OK;
}

/* Linux 6.18.44 x86_64 fixture encodes CONFIG_MODVERSIONS records as
 * unsigned-long CRC plus a 56-byte symbol name, i.e. 64 bytes total. */
struct selinos_modversion_info {
    selinos_u64 crc;
    char name[56];
};

int selinos_kabi_verify_module_versions(const selinos_u8 *image,
                                        selinos_size_t image_size,
                                        const char *required_vermagic_prefix,
                                        selinos_kabi_crc_resolver_t resolve_crc,
                                        void *resolve_context)
{
    struct selinos_kabi_module_summary summary;
    const struct selinos_elf64_ehdr *header;
    const struct selinos_elf64_shdr *sections;
    const char *section_names;
    selinos_size_t section_names_size;
    selinos_u16 index;

    if (resolve_crc == SELINOS_NULL) {
        return SELINOS_KABI_MODULE_E_ARGUMENT;
    }
    if (selinos_kabi_parse_module(image, image_size, required_vermagic_prefix, &summary) !=
        SELINOS_KABI_MODULE_OK || !summary.has_versions) {
        return SELINOS_KABI_MODULE_E_METADATA;
    }
    header = (const struct selinos_elf64_ehdr *)image;
    sections = (const struct selinos_elf64_shdr *)(image + header->shoff);
    section_names = (const char *)(image + sections[header->shstrndx].offset);
    section_names_size = (selinos_size_t)sections[header->shstrndx].size;

    for (index = 0U; index < header->shnum; ++index) {
        const struct selinos_elf64_shdr *section = &sections[index];
        const char *name;
        selinos_size_t name_limit;
        const struct selinos_modversion_info *records;
        selinos_size_t record_count;
        selinos_size_t record_index;

        if (section->name >= section_names_size) {
            return SELINOS_KABI_MODULE_E_SECTIONS;
        }
        name = section_names + section->name;
        name_limit = section_names_size - section->name;
        if (!name_equals(name, name_limit, "__versions")) {
            continue;
        }
        if (section->size == 0U ||
            section->size % sizeof(struct selinos_modversion_info) != 0U ||
            !range_ok(image_size, section->offset, section->size)) {
            return SELINOS_KABI_MODULE_E_METADATA;
        }
        records = (const struct selinos_modversion_info *)(image + section->offset);
        record_count = (selinos_size_t)(section->size / sizeof(struct selinos_modversion_info));
        for (record_index = 0U; record_index < record_count; ++record_index) {
            selinos_u32 expected_crc;
            if (!string_terminated(records[record_index].name,
                                   sizeof(records[record_index].name)) ||
                records[record_index].name[0] == '\0' ||
                !resolve_crc(records[record_index].name, &expected_crc, resolve_context) ||
                records[record_index].crc != (selinos_u64)expected_crc) {
                return SELINOS_KABI_MODULE_E_SYMBOL;
            }
        }
        return SELINOS_KABI_MODULE_OK;
    }
    return SELINOS_KABI_MODULE_E_METADATA;
}
