// SPDX-License-Identifier: MIT
#include "selinos_elfrt.h"

#define ELFRT_EI_NIDENT 16u
#define ELFRT_ELFCLASS64 2u
#define ELFRT_ELFDATA2LSB 1u
#define ELFRT_EV_CURRENT 1u
#define ELFRT_ET_EXEC 2u
#define ELFRT_ET_DYN 3u
#define ELFRT_EM_X86_64 62u
#define ELFRT_PT_LOAD 1u
#define ELFRT_PT_DYNAMIC 2u
#define ELFRT_PT_INTERP 3u
#define ELFRT_PT_GNU_STACK 0x6474e551u
#define ELFRT_PF_X 1u
#define ELFRT_PF_W 2u
#define ELFRT_PF_R 4u
#define ELFRT_DT_NULL 0
#define ELFRT_DT_NEEDED 1
#define ELFRT_DT_STRTAB 5
#define ELFRT_DT_RELA 7
#define ELFRT_DT_STRSZ 10
#define ELFRT_DT_TEXTREL 22
#define ELFRT_DT_FLAGS 30
#define ELFRT_DF_TEXTREL 4u

struct elfrt_ehdr {
    selinos_elfrt_u8 ident[ELFRT_EI_NIDENT];
    selinos_elfrt_u16 type;
    selinos_elfrt_u16 machine;
    selinos_elfrt_u32 version;
    selinos_elfrt_u64 entry;
    selinos_elfrt_u64 phoff;
    selinos_elfrt_u64 shoff;
    selinos_elfrt_u32 flags;
    selinos_elfrt_u16 ehsize;
    selinos_elfrt_u16 phentsize;
    selinos_elfrt_u16 phnum;
    selinos_elfrt_u16 shentsize;
    selinos_elfrt_u16 shnum;
    selinos_elfrt_u16 shstrndx;
};

struct elfrt_phdr {
    selinos_elfrt_u32 type;
    selinos_elfrt_u32 flags;
    selinos_elfrt_u64 offset;
    selinos_elfrt_u64 vaddr;
    selinos_elfrt_u64 paddr;
    selinos_elfrt_u64 filesz;
    selinos_elfrt_u64 memsz;
    selinos_elfrt_u64 align;
};

struct elfrt_dyn {
    selinos_elfrt_s64 tag;
    union {
        selinos_elfrt_u64 value;
        selinos_elfrt_u64 pointer;
    } un;
};

static int range_valid(selinos_elfrt_size_t total, selinos_elfrt_u64 offset,
                       selinos_elfrt_u64 length)
{
    return offset <= total && length <= total - offset;
}

static void clear_summary(struct selinos_elfrt_summary *summary)
{
    selinos_elfrt_u8 *bytes = (selinos_elfrt_u8 *)summary;
    for (selinos_elfrt_size_t i = 0u; i < sizeof(*summary); ++i) {
        bytes[i] = 0u;
    }
}

static int power_of_two(selinos_elfrt_u64 value)
{
    return value != 0u && (value & (value - 1u)) == 0u;
}

static int copy_bounded_string(char *destination, selinos_elfrt_size_t capacity,
                               const selinos_elfrt_u8 *source,
                               selinos_elfrt_size_t available)
{
    if (capacity == 0u || available == 0u) {
        return 0;
    }
    selinos_elfrt_size_t i = 0u;
    while (i + 1u < capacity && i < available && source[i] != '\0') {
        destination[i] = (char)source[i];
        ++i;
    }
    if (i == available || source[i] != '\0') {
        return 0;
    }
    destination[i] = '\0';
    return 1;
}

static int virtual_to_file_offset(const struct elfrt_phdr *headers,
                                  selinos_elfrt_u16 count,
                                  selinos_elfrt_u64 virtual_address,
                                  selinos_elfrt_u64 *file_offset,
                                  selinos_elfrt_u64 *available)
{
    for (selinos_elfrt_u16 i = 0u; i < count; ++i) {
        const struct elfrt_phdr *header = &headers[i];
        if (header->type != ELFRT_PT_LOAD || virtual_address < header->vaddr) {
            continue;
        }
        const selinos_elfrt_u64 delta = virtual_address - header->vaddr;
        if (delta < header->filesz) {
            *file_offset = header->offset + delta;
            *available = header->filesz - delta;
            return 1;
        }
    }
    return 0;
}

int selinos_elfrt_parse_image(const selinos_elfrt_u8 *image,
                              selinos_elfrt_size_t image_size,
                              int allow_interpreter,
                              struct selinos_elfrt_summary *summary)
{
    const struct elfrt_ehdr *header;
    const struct elfrt_phdr *program_headers;
    const struct elfrt_phdr *dynamic_header = 0;
    const struct elfrt_phdr *gnu_stack_header = 0;
    selinos_elfrt_u64 string_table_address = 0u;
    selinos_elfrt_u64 string_table_size = 0u;
    selinos_elfrt_u64 needed_offsets[SELINOS_ELFRT_MAX_NEEDED];
    selinos_elfrt_u32 needed_count = 0u;

    if (image == 0 || summary == 0) {
        return SELINOS_ELFRT_E_ARGUMENT;
    }
    clear_summary(summary);
    if (image_size < sizeof(*header)) {
        return SELINOS_ELFRT_E_TRUNCATED;
    }
    header = (const struct elfrt_ehdr *)image;
    if (header->ident[0] != 0x7fu || header->ident[1] != 'E' ||
        header->ident[2] != 'L' || header->ident[3] != 'F' ||
        header->ident[4] != ELFRT_ELFCLASS64 ||
        header->ident[5] != ELFRT_ELFDATA2LSB ||
        header->ident[6] != ELFRT_EV_CURRENT ||
        header->version != ELFRT_EV_CURRENT ||
        header->ehsize != sizeof(*header)) {
        return SELINOS_ELFRT_E_ELF;
    }
    if (header->machine != ELFRT_EM_X86_64) {
        return SELINOS_ELFRT_E_ARCH;
    }
    if (header->type != ELFRT_ET_EXEC && header->type != ELFRT_ET_DYN) {
        return SELINOS_ELFRT_E_TYPE;
    }
    if (header->phnum == 0u || header->phnum > 64u ||
        header->phentsize != sizeof(struct elfrt_phdr) ||
        !range_valid(image_size, header->phoff,
                     (selinos_elfrt_u64)header->phnum * header->phentsize)) {
        return SELINOS_ELFRT_E_PROGRAM_HEADERS;
    }
    program_headers = (const struct elfrt_phdr *)(image + header->phoff);
    summary->elf_type = header->type;
    summary->elf_machine = header->machine;
    summary->entry = header->entry;

    for (selinos_elfrt_u16 i = 0u; i < header->phnum; ++i) {
        const struct elfrt_phdr *program = &program_headers[i];
        if (program->type == ELFRT_PT_LOAD) {
            if (program->filesz > program->memsz ||
                !range_valid(image_size, program->offset, program->filesz) ||
                (program->align > 1u &&
                 (!power_of_two(program->align) ||
                  ((program->vaddr - program->offset) & (program->align - 1u)) != 0u))) {
                return SELINOS_ELFRT_E_SEGMENT;
            }
            if (summary->load_segments == SELINOS_ELFRT_MAX_LOAD_SEGMENTS) {
                return SELINOS_ELFRT_E_POLICY;
            }
            ++summary->load_segments;
            if ((program->flags & ELFRT_PF_W) != 0u) {
                ++summary->writable_load_segments;
            }
            if ((program->flags & ELFRT_PF_X) != 0u) {
                ++summary->executable_load_segments;
            }
            if ((program->flags & (ELFRT_PF_W | ELFRT_PF_X)) ==
                (ELFRT_PF_W | ELFRT_PF_X)) {
                ++summary->writable_executable_load_segments;
            }
        } else if (program->type == ELFRT_PT_INTERP) {
            if (summary->has_interp != 0u || !allow_interpreter ||
                !range_valid(image_size, program->offset, program->filesz) ||
                !copy_bounded_string(summary->interpreter, sizeof(summary->interpreter),
                                     image + program->offset, program->filesz)) {
                return SELINOS_ELFRT_E_POLICY;
            }
            summary->has_interp = 1u;
        } else if (program->type == ELFRT_PT_DYNAMIC) {
            if (dynamic_header != 0 || !range_valid(image_size, program->offset,
                                                      program->filesz) ||
                program->filesz % sizeof(struct elfrt_dyn) != 0u) {
                return SELINOS_ELFRT_E_DYNAMIC;
            }
            dynamic_header = program;
            summary->has_dynamic = 1u;
        } else if (program->type == ELFRT_PT_GNU_STACK) {
            if (gnu_stack_header != 0 ||
                program->flags != (ELFRT_PF_R | ELFRT_PF_W) ||
                program->offset != 0u || program->vaddr != 0u ||
                program->paddr != 0u || program->filesz != 0u ||
                program->memsz != 0u ||
                (program->align != 0u && !power_of_two(program->align))) {
                return SELINOS_ELFRT_E_POLICY;
            }
            gnu_stack_header = program;
            summary->has_gnu_stack = 1u;
            summary->gnu_stack_flags = program->flags;
        }
    }
    if (summary->load_segments == 0u) {
        return SELINOS_ELFRT_E_SEGMENT;
    }
    if (dynamic_header == 0) {
        return SELINOS_ELFRT_OK;
    }

    const struct elfrt_dyn *dynamic = (const struct elfrt_dyn *)(image + dynamic_header->offset);
    const selinos_elfrt_u64 entries = dynamic_header->filesz / sizeof(*dynamic);
    int found_null = 0;
    for (selinos_elfrt_u64 i = 0u; i < entries; ++i) {
        if (dynamic[i].tag == ELFRT_DT_NULL) {
            found_null = 1;
            break;
        }
        if (dynamic[i].tag == ELFRT_DT_STRTAB) {
            string_table_address = dynamic[i].un.pointer;
        } else if (dynamic[i].tag == ELFRT_DT_STRSZ) {
            string_table_size = dynamic[i].un.value;
        } else if (dynamic[i].tag == ELFRT_DT_NEEDED) {
            if (needed_count == SELINOS_ELFRT_MAX_NEEDED) {
                return SELINOS_ELFRT_E_POLICY;
            }
            needed_offsets[needed_count++] = dynamic[i].un.value;
        } else if (dynamic[i].tag == ELFRT_DT_RELA) {
            summary->has_rela = 1u;
        } else if (dynamic[i].tag == ELFRT_DT_TEXTREL ||
                   (dynamic[i].tag == ELFRT_DT_FLAGS &&
                    (dynamic[i].un.value & ELFRT_DF_TEXTREL) != 0u)) {
            summary->has_textrel = 1u;
        }
    }
    if (!found_null) {
        return SELINOS_ELFRT_E_DYNAMIC;
    }
    summary->needed_count = needed_count;
    if (needed_count == 0u) {
        return SELINOS_ELFRT_OK;
    }
    if (string_table_address == 0u || string_table_size == 0u) {
        return SELINOS_ELFRT_E_DYNAMIC;
    }
    selinos_elfrt_u64 string_table_offset;
    selinos_elfrt_u64 string_table_available;
    if (!virtual_to_file_offset(program_headers, header->phnum, string_table_address,
                                &string_table_offset, &string_table_available) ||
        string_table_size > string_table_available ||
        !range_valid(image_size, string_table_offset, string_table_size)) {
        return SELINOS_ELFRT_E_DYNAMIC;
    }
    for (selinos_elfrt_u32 i = 0u; i < needed_count; ++i) {
        if (needed_offsets[i] >= string_table_size ||
            !copy_bounded_string(summary->needed[i], sizeof(summary->needed[i]),
                                 image + string_table_offset + needed_offsets[i],
                                 string_table_size - needed_offsets[i])) {
            return SELINOS_ELFRT_E_DYNAMIC;
        }
    }
    return SELINOS_ELFRT_OK;
}

int selinos_elfrt_validate_initial_load_policy(const struct selinos_elfrt_summary *summary)
{
    if (summary == 0) {
        return SELINOS_ELFRT_E_ARGUMENT;
    }
    if (summary->has_textrel != 0u ||
        summary->writable_executable_load_segments != 0u) {
        return SELINOS_ELFRT_E_POLICY;
    }
    return SELINOS_ELFRT_OK;
}
