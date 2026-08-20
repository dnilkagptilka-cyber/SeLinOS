// SPDX-License-Identifier: MIT
#ifndef SELINOS_ELFRT_H
#define SELINOS_ELFRT_H

/* Freestanding x86_64 ELF runtime metadata parser. This component validates
 * an untrusted byte buffer only; it maps nothing, resolves no symbol, invokes
 * no interpreter and does not execute code. */
typedef unsigned char selinos_elfrt_u8;
typedef unsigned short selinos_elfrt_u16;
typedef unsigned int selinos_elfrt_u32;
typedef unsigned long selinos_elfrt_u64;
typedef long selinos_elfrt_s64;
typedef unsigned long selinos_elfrt_size_t;

#define SELINOS_ELFRT_MAX_LOAD_SEGMENTS 8u
#define SELINOS_ELFRT_MAX_NEEDED 8u
#define SELINOS_ELFRT_INTERP_MAX 96u
#define SELINOS_ELFRT_NEEDED_NAME_MAX 96u

struct selinos_elfrt_summary {
    selinos_elfrt_u16 elf_type;
    selinos_elfrt_u16 elf_machine;
    selinos_elfrt_u64 entry;
    selinos_elfrt_u32 load_segments;
    selinos_elfrt_u32 writable_load_segments;
    selinos_elfrt_u32 executable_load_segments;
    selinos_elfrt_u32 writable_executable_load_segments;
    selinos_elfrt_u32 needed_count;
    selinos_elfrt_u32 has_dynamic;
    selinos_elfrt_u32 has_interp;
    selinos_elfrt_u32 has_rela;
    selinos_elfrt_u32 has_textrel;
    selinos_elfrt_u32 has_gnu_stack;
    selinos_elfrt_u32 gnu_stack_flags;
    selinos_elfrt_u32 has_note;
    selinos_elfrt_u32 note_size;
    char interpreter[SELINOS_ELFRT_INTERP_MAX];
    char needed[SELINOS_ELFRT_MAX_NEEDED][SELINOS_ELFRT_NEEDED_NAME_MAX];
};

enum selinos_elfrt_status {
    SELINOS_ELFRT_OK = 0,
    SELINOS_ELFRT_E_ARGUMENT = -1,
    SELINOS_ELFRT_E_TRUNCATED = -2,
    SELINOS_ELFRT_E_ELF = -3,
    SELINOS_ELFRT_E_ARCH = -4,
    SELINOS_ELFRT_E_TYPE = -5,
    SELINOS_ELFRT_E_PROGRAM_HEADERS = -6,
    SELINOS_ELFRT_E_SEGMENT = -7,
    SELINOS_ELFRT_E_DYNAMIC = -8,
    SELINOS_ELFRT_E_POLICY = -9
};

/* Accepts exactly ELF64 little-endian x86_64 ET_EXEC or ET_DYN. It validates
 * file bounds and PT_LOAD alignment/ranges, then parses bounded PT_INTERP,
 * PT_DYNAMIC, one fail-closed PT_GNU_STACK record and one bounded PT_NOTE record.
 * `allow_interpreter` controls only acceptance of a
 * PT_INTERP record; no interpreter is opened or executed. */
int selinos_elfrt_parse_image(const selinos_elfrt_u8 *image,
                              selinos_elfrt_size_t image_size,
                              int allow_interpreter,
                              struct selinos_elfrt_summary *summary);

/* Initial mapping policy for a future loader. The current pinned x86_64 seL4
 * profile has no proven execute-disable control, so any text relocation or
 * writable-plus-executable segment is rejected before mapping is attempted. */
int selinos_elfrt_validate_initial_load_policy(const struct selinos_elfrt_summary *summary);

#endif
