// SPDX-License-Identifier: MIT
#include "selinos_execve_reply_stack_read_m0_fixture.h"
#include "selinos_execve_reply_stack_read_m0_protocol.h"

/* ELF64 little-endian x86_64 ET_DYN with four page-aligned PT_LOAD records,
 * one PT_DYNAMIC relative-relocation table and fixed PT_GNU_STACK metadata:
 * a five-byte entry RX jump at 0x60000000, RO data at 0x60002000, RW+BSS at
 * 0x60001000, and a second RX page at 0x60003000. The jump is required before
 * the second RX code may read data, verify and update BSS, and reach UD2. */
static const selinos_elfrt_u8 static_elf_two_rx_ro_rw_bss_fixture[
    SELINOS_EXECVE_REPLY_STACK_READ_M0_IMAGE_BYTES] = {
    [0] = 0x7f, [1] = 'E', [2] = 'L', [3] = 'F',
    [4] = 2u, [5] = 1u, [6] = 1u,
    [16] = 3u,                          [18] = 0x3eu,                      /* EM_X86_64 */
    [20] = 1u,                         /* EV_CURRENT */
    /* e_entry = relative offset zero; root selects fixed nonzero load base. */
    [32] = 0x40u,                      /* e_phoff = 64 */
    [52] = 0x40u,                      /* e_ehsize = 64 */
    [54] = 0x38u,                      /* e_phentsize = 56 */
    [56] = 7u,                         /* e_phnum = 7 */

    [64] = 1u,                         /* PT_LOAD entry RX */
    [68] = 5u,                         /* PF_R | PF_X */
    [73] = 0x10u,                      /* p_offset = 0x1000 */
    /* p_vaddr = relative offset zero */
    [96] = SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_PAYLOAD_BYTES,
    [104] = SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_PAYLOAD_BYTES,
    [113] = 0x10u,                     /* p_align = 0x1000 */

    [120] = 1u,                        /* PT_LOAD read-only data */
    [124] = 4u,                        /* PF_R */
    [129] = 0x20u,                     /* p_offset = 0x2000 */
    [137] = 0x20u,                     /* p_vaddr = relative 0x2000 */
    [152] = 0x18u, [153] = 0x01u,      /* p_filesz = 280 */
    [160] = 0x18u, [161] = 0x01u,      /* p_memsz = 280 */
    [169] = 0x10u,                     /* p_align = 0x1000 */

    [176] = 1u,                        /* PT_LOAD writable data */
    [180] = 6u,                        /* PF_R | PF_W */
    [185] = 0x30u,                     /* p_offset = 0x3000 */
    [193] = 0x10u,                     /* p_vaddr = relative 0x1000 */
    [208] = SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_INITIALIZED_BYTES,
    [216] = SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_MEMORY_BYTES,
    [225] = 0x10u,                     /* p_align = 0x1000 */

    [232] = 1u,                        /* PT_LOAD second RX */
    [236] = 5u,                        /* PF_R | PF_X */
    [241] = 0x40u,                     /* p_offset = 0x4000 */
    [249] = 0x30u,                     /* p_vaddr = relative 0x3000 */
    [264] = SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_PAYLOAD_BYTES,
    [272] = SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_PAYLOAD_BYTES,
    [281] = 0x10u,                     /* p_align = 0x1000 */

    [288] = 0x51u, [289] = 0xe5u, [290] = 0x74u, [291] = 0x64u,
                                          /* PT_GNU_STACK = 0x6474e551 */
    [292] = 6u,                         /* PF_R | PF_W, no PF_X */

    [344] = 2u,                         /* PT_DYNAMIC */
    [348] = 4u,                         /* PF_R */
    [352] = 0x40u, [353] = 0x20u,       /* p_offset = 0x2040 */
    [360] = 0x40u, [361] = 0x20u,       /* p_vaddr = relative 0x2040 */
    [368] = 0x40u, [369] = 0x20u,       /* p_paddr = relative 0x2040 */
    [376] = 64u, [384] = 64u,            /* four dynamic entries */
    [392] = 8u,                         /* p_align = 8 */

    [400] = 3u,                         /* PT_INTERP */
    [404] = 4u,                         /* PF_R */
    [408] = 0x80u, [409] = 0x02u,       /* p_offset = 0x280 */
    [424] = 24u, [432] = 24u,            /* bounded pathname including NUL */
    [448] = 1u,                         /* p_align = 1 */

    /* mov eax, Linux execve(59); syscall. Root replaces this fixed context. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_LOAD_OFFSET + 0u] = 0xb8u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_LOAD_OFFSET + 1u] =
        SELINOS_EXECVE_REPLY_STACK_READ_M0_LINUX_EXECVE_SYSCALL,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_LOAD_OFFSET + 5u] = 0x0fu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_LOAD_OFFSET + 6u] = 0x05u,

    /* Second RX: movabs rax, 0x60001000. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 0u] = 0x48u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 1u] = 0xb8u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 3u] = 0x10u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 5u] = 0x60u,
    /* mov edx, [rax]; cmp edx, 0x534c3832; jne failure. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 10u] = 0x8bu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 11u] = 0x10u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 12u] = 0x81u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 13u] = 0xfau,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 14u] = 0x32u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 15u] = 0x38u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 16u] = 0x4cu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 17u] = 0x53u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 18u] = 0x75u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 19u] = 0x37u,
    /* cmp byte [rax+4], 0; jne failure. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 20u] = 0x80u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 21u] = 0x78u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 22u] = 0x04u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 24u] = 0x75u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 25u] = 0x31u,
    /* Read relocated pointer at data+8 and require fixed load-base + 0x3000. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 26u] = 0x48u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 27u] = 0x8bu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 28u] = 0x50u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 29u] = 0x08u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 30u] = 0x48u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 31u] = 0xb9u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 33u] = 0x30u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 35u] = 0x60u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 40u] = 0x48u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 41u] = 0x39u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 42u] = 0xcau,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 43u] = 0x75u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 44u] = 0x1eu,
    /* movabs rcx, 0x60002000; mov edx, [rcx]. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 45u] = 0x48u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 46u] = 0xb9u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 48u] = 0x20u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 50u] = 0x60u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 55u] = 0x8bu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 56u] = 0x11u,
    /* cmp edx, 0x524f3832; jne failure. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 57u] = 0x81u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 58u] = 0xfau,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 59u] = 0x32u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 60u] = 0x38u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 61u] = 0x4fu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 62u] = 0x52u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 63u] = 0x75u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 64u] = 0x0au,
    /* mov byte [rax+4], 0xa6; cmp byte [rax+4], 0xa6; je success. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 65u] = 0xc6u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 66u] = 0x40u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 67u] = 0x04u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 68u] =
        SELINOS_EXECVE_REPLY_STACK_READ_M0_BSS_WRITE_VALUE,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 69u] = 0x80u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 70u] = 0x78u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 71u] = 0x04u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 72u] =
        SELINOS_EXECVE_REPLY_STACK_READ_M0_BSS_WRITE_VALUE,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 73u] = 0x74u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET + 74u] = 0x0eu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET +
     SELINOS_EXECVE_REPLY_STACK_READ_M0_FAILURE_INT3_OFFSET] = 0xccu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET +
     SELINOS_EXECVE_REPLY_STACK_READ_M0_SUCCESS_UD2_OFFSET] = 0x0fu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET +
     SELINOS_EXECVE_REPLY_STACK_READ_M0_SUCCESS_UD2_OFFSET + 1u] = 0x0bu,

    /* Exact non-host interpreter witness pathname. */
    [640] = '/', [641] = 's', [642] = 'e', [643] = 'l', [644] = 'i',
    [645] = 'n', [646] = 'o', [647] = 's', [648] = '/', [649] = 'p',
    [650] = 'h', [651] = 'a', [652] = 's', [653] = 'e', [654] = '8',
    [655] = '6', [656] = '-', [657] = 'i', [658] = 'n', [659] = 't',
    [660] = 'e', [661] = 'r', [662] = 'p', [663] = '\0',

    /* DT_RELA=0x2100, DT_RELASZ=24, DT_RELAENT=24, DT_NULL. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 0u] = 7u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 9u] = 0x21u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 16u] = 8u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 24u] = 24u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 32u] = 9u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DYNAMIC_OFFSET + 40u] = 24u,
    /* Elf64_Rela: r_offset=0x1008, R_X86_64_RELATIVE, r_addend=0x3000. */
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RELA_OFFSET + 1u] = 0x10u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RELA_OFFSET + 8u] =
        SELINOS_EXECVE_REPLY_STACK_READ_M0_RELA_TYPE,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RELA_OFFSET + 16u] = 0x30u,

    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RO_LOAD_OFFSET + 0u] = 0x32u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RO_LOAD_OFFSET + 1u] = 0x38u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RO_LOAD_OFFSET + 2u] = 0x4fu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_RO_LOAD_OFFSET + 3u] = 0x52u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_LOAD_OFFSET + 0u] = 0x32u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_LOAD_OFFSET + 1u] = 0x38u,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_LOAD_OFFSET + 2u] = 0x4cu,
    [SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_LOAD_OFFSET + 3u] = 0x53u,
};

const selinos_elfrt_u8 *selinos_execve_reply_stack_read_m0_fixture(
    selinos_elfrt_size_t *image_bytes)
{
    if (image_bytes != 0) {
        *image_bytes = sizeof(static_elf_two_rx_ro_rw_bss_fixture);
    }
    return static_elf_two_rx_ro_rw_bss_fixture;
}

int selinos_execve_reply_stack_read_m0_validate_fixture(
    const selinos_elfrt_u8 *image, selinos_elfrt_size_t image_bytes,
    struct selinos_elfrt_summary *summary)
{
    selinos_elfrt_size_t index;

    if (image == 0 || summary == 0 ||
        image_bytes != SELINOS_EXECVE_REPLY_STACK_READ_M0_IMAGE_BYTES) {
        return SELINOS_ELFRT_E_ARGUMENT;
    }
    for (index = 0u; index < image_bytes; ++index) {
        if (image[index] != static_elf_two_rx_ro_rw_bss_fixture[index]) {
            return SELINOS_ELFRT_E_POLICY;
        }
    }
    if (selinos_elfrt_parse_image(image, image_bytes, 1, summary) !=
            SELINOS_ELFRT_OK ||
        selinos_elfrt_validate_initial_load_policy(summary) != SELINOS_ELFRT_OK ||
        summary->elf_type != 3u || summary->elf_machine != 62u ||
        summary->entry != 0u ||
        summary->load_segments != 4u || summary->writable_load_segments != 1u ||
        summary->executable_load_segments != 2u ||
        summary->writable_executable_load_segments != 0u ||
        summary->has_gnu_stack != 1u || summary->gnu_stack_flags != 6u ||
        summary->has_note != 0u || summary->needed_count != 0u ||
        summary->has_dynamic != 1u || summary->has_interp != 1u ||
        summary->interpreter[0] != '/' || summary->interpreter[23] != '\0' ||
        summary->has_rela != 1u ||
        summary->rela_address != 0x2100u || summary->rela_size != 24u ||
        summary->rela_entry_size != 24u ||
        summary->has_textrel != 0u ||
        image[SELINOS_EXECVE_REPLY_STACK_READ_M0_ENTRY_LOAD_OFFSET] != 0xb8u ||
        image[SELINOS_EXECVE_REPLY_STACK_READ_M0_SECOND_RX_LOAD_OFFSET +
              SELINOS_EXECVE_REPLY_STACK_READ_M0_SUCCESS_UD2_OFFSET] != 0x0fu ||
        image[SELINOS_EXECVE_REPLY_STACK_READ_M0_RO_LOAD_OFFSET] != 0x32u ||
        image[SELINOS_EXECVE_REPLY_STACK_READ_M0_DATA_LOAD_OFFSET] != 0x32u) {
        return SELINOS_ELFRT_E_POLICY;
    }
    return SELINOS_ELFRT_OK;
}
