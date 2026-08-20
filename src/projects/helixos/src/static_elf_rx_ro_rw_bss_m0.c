// SPDX-License-Identifier: MIT
#include "selinos_static_elf_rx_ro_rw_bss_m0_fixture.h"
#include "selinos_static_elf_rx_ro_rw_bss_m0_protocol.h"

/* ELF64 little-endian x86_64 ET_EXEC with three page-aligned PT_LOAD records:
 * RX text at 0x60000000, an independent RO constant page at 0x60002000, and
 * initialized RW data plus a bounded BSS tail at 0x60001000. Text reads both
 * data values, requires BSS zero, writes/rereads one BSS byte, then reaches UD2. */
static const selinos_elfrt_u8 static_elf_rx_ro_rw_bss_fixture[
    SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_IMAGE_BYTES] = {
    [0] = 0x7f, [1] = 'E', [2] = 'L', [3] = 'F',
    [4] = 2u, [5] = 1u, [6] = 1u,
    [16] = 2u,                         /* ET_EXEC */
    [18] = 0x3eu,                      /* EM_X86_64 */
    [20] = 1u,                         /* EV_CURRENT */
    [27] = 0x60u,                      /* e_entry = 0x60000000 */
    [32] = 0x40u,                      /* e_phoff = 64 */
    [52] = 0x40u,                      /* e_ehsize = 64 */
    [54] = 0x38u,                      /* e_phentsize = 56 */
    [56] = 3u,                         /* e_phnum = 3 */

    [64] = 1u,                         /* PT_LOAD text */
    [68] = 5u,                         /* PF_R | PF_X */
    [73] = 0x10u,                      /* p_offset = 0x1000 */
    [83] = 0x60u,                      /* p_vaddr = 0x60000000 */
    [96] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_PAYLOAD_BYTES,
    [104] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_PAYLOAD_BYTES,
    [113] = 0x10u,                     /* p_align = 0x1000 */

    [120] = 1u,                        /* PT_LOAD read-only data */
    [124] = 4u,                        /* PF_R */
    [129] = 0x20u,                     /* p_offset = 0x2000 */
    [137] = 0x20u, [139] = 0x60u,      /* p_vaddr = 0x60002000 */
    [152] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_INITIALIZED_BYTES,
    [160] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_INITIALIZED_BYTES,
    [169] = 0x10u,                     /* p_align = 0x1000 */

    [176] = 1u,                        /* PT_LOAD writable data */
    [180] = 6u,                        /* PF_R | PF_W */
    [185] = 0x30u,                     /* p_offset = 0x3000 */
    [193] = 0x10u, [195] = 0x60u,      /* p_vaddr = 0x60001000 */
    [208] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_INITIALIZED_BYTES,
    [216] = SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_MEMORY_BYTES,
    [225] = 0x10u,                     /* p_align = 0x1000 */

    /* movabs rax, 0x60001000 */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 0u] = 0x48u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 1u] = 0xb8u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 2u] = 0x00u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 3u] = 0x10u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 4u] = 0x00u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 5u] = 0x60u,
    /* mov edx, [rax]; cmp edx, 0x534c3831; jne failure */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 10u] = 0x8bu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 11u] = 0x10u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 12u] = 0x81u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 13u] = 0xfau,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 14u] = 0x31u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 15u] = 0x38u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 16u] = 0x4cu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 17u] = 0x53u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 18u] = 0x75u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 19u] = 0x24u,
    /* cmp byte [rax+4], 0; jne failure */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 20u] = 0x80u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 21u] = 0x78u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 22u] = 0x04u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 23u] = 0x00u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 24u] = 0x75u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 25u] = 0x1eu,
    /* movabs rcx, 0x60002000; mov edx, [rcx] */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 26u] = 0x48u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 27u] = 0xb9u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 28u] = 0x00u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 29u] = 0x20u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 30u] = 0x00u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 31u] = 0x60u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 36u] = 0x8bu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 37u] = 0x11u,
    /* cmp edx, 0x524f3831; jne failure */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 38u] = 0x81u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 39u] = 0xfau,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 40u] = 0x31u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 41u] = 0x38u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 42u] = 0x4fu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 43u] = 0x52u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 44u] = 0x75u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 45u] = 0x0au,
    /* mov byte [rax+4], 0xa5; cmp byte [rax+4], 0xa5; je success */
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 46u] = 0xc6u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 47u] = 0x40u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 48u] = 0x04u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 49u] =
        SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_BSS_WRITE_VALUE,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 50u] = 0x80u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 51u] = 0x78u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 52u] = 0x04u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 53u] =
        SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_BSS_WRITE_VALUE,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 54u] = 0x74u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 55u] = 0x0eu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET +
     SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_FAILURE_INT3_OFFSET] = 0xccu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 57u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 58u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 59u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 60u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 61u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 62u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 63u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 64u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 65u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 66u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 67u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 68u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET + 69u] = 0x90u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET +
     SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_SUCCESS_UD2_OFFSET] = 0x0fu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_TEXT_LOAD_OFFSET +
     SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_SUCCESS_UD2_OFFSET + 1u] = 0x0bu,

    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 0u] = 0x31u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 1u] = 0x38u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 2u] = 0x4fu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 3u] = 0x52u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 0u] = 0x31u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 1u] = 0x38u,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 2u] = 0x4cu,
    [SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 3u] = 0x53u,
};

const selinos_elfrt_u8 *selinos_static_elf_rx_ro_rw_bss_m0_fixture(
    selinos_elfrt_size_t *image_bytes)
{
    if (image_bytes != 0) {
        *image_bytes = sizeof(static_elf_rx_ro_rw_bss_fixture);
    }
    return static_elf_rx_ro_rw_bss_fixture;
}

int selinos_static_elf_rx_ro_rw_bss_m0_validate_fixture(
    const selinos_elfrt_u8 *image, selinos_elfrt_size_t image_bytes,
    struct selinos_elfrt_summary *summary)
{
    selinos_elfrt_size_t index;

    if (image == 0 || summary == 0 ||
        image_bytes != SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_IMAGE_BYTES) {
        return SELINOS_ELFRT_E_ARGUMENT;
    }
    for (index = 0u; index < image_bytes; ++index) {
        if (image[index] != static_elf_rx_ro_rw_bss_fixture[index]) {
            return SELINOS_ELFRT_E_POLICY;
        }
    }
    if (selinos_elfrt_parse_image(image, image_bytes, 0, summary) !=
            SELINOS_ELFRT_OK ||
        selinos_elfrt_validate_initial_load_policy(summary) != SELINOS_ELFRT_OK ||
        summary->elf_type != 2u || summary->elf_machine != 62u ||
        summary->entry != SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_FIXED_ENTRY_VADDR ||
        summary->load_segments != 3u || summary->writable_load_segments != 1u ||
        summary->executable_load_segments != 1u ||
        summary->writable_executable_load_segments != 0u ||
        summary->needed_count != 0u || summary->has_dynamic != 0u ||
        summary->has_interp != 0u || summary->has_rela != 0u ||
        summary->has_textrel != 0u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET] != 0x31u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 1u] != 0x38u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 2u] != 0x4fu ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + 3u] != 0x52u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET] != 0x31u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 1u] != 0x38u ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 2u] != 0x4cu ||
        image[SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + 3u] != 0x53u) {
        return SELINOS_ELFRT_E_POLICY;
    }
    return SELINOS_ELFRT_OK;
}
