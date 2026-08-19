// SPDX-License-Identifier: MIT
#include "selinos_static_elf_rx_load_m0_fixture.h"
#include "selinos_static_elf_rx_load_m0_protocol.h"

/* ELF64 little-endian x86_64 ET_EXEC, one PT_LOAD at file offset 0x1000 and
 * virtual address 0x60000000. The loadable bytes are only UD2. */
static const selinos_elfrt_u8 static_elf_rx_fixture[
    SELINOS_STATIC_ELF_RX_LOAD_M0_IMAGE_BYTES] = {
    [0] = 0x7f, [1] = 'E', [2] = 'L', [3] = 'F',
    [4] = 2u, [5] = 1u, [6] = 1u,
    [16] = 2u,                         /* ET_EXEC */
    [18] = 0x3eu,                      /* EM_X86_64 */
    [20] = 1u,                         /* EV_CURRENT */
    [27] = 0x60u,                      /* e_entry = 0x60000000 */
    [32] = 0x40u,                      /* e_phoff = 64 */
    [52] = 0x40u,                      /* e_ehsize = 64 */
    [54] = 0x38u,                      /* e_phentsize = 56 */
    [56] = 1u,                         /* e_phnum = 1 */

    [64] = 1u,                         /* PT_LOAD */
    [68] = 5u,                         /* PF_R | PF_X */
    [73] = 0x10u,                      /* p_offset = 0x1000 */
    [83] = 0x60u,                      /* p_vaddr = 0x60000000 */
    [96] = SELINOS_STATIC_ELF_RX_LOAD_M0_PAYLOAD_BYTES,
    [104] = SELINOS_STATIC_ELF_RX_LOAD_M0_PAYLOAD_BYTES,
    [113] = 0x10u,                     /* p_align = 0x1000 */

    [SELINOS_STATIC_ELF_RX_LOAD_M0_LOAD_OFFSET] =
        SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE0,
    [SELINOS_STATIC_ELF_RX_LOAD_M0_LOAD_OFFSET + 1u] =
        SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE1,
};

const selinos_elfrt_u8 *selinos_static_elf_rx_load_m0_fixture(
    selinos_elfrt_size_t *image_bytes)
{
    if (image_bytes != 0) {
        *image_bytes = sizeof(static_elf_rx_fixture);
    }
    return static_elf_rx_fixture;
}

int selinos_static_elf_rx_load_m0_validate_fixture(
    const selinos_elfrt_u8 *image, selinos_elfrt_size_t image_bytes,
    struct selinos_elfrt_summary *summary)
{
    selinos_elfrt_size_t index;

    if (image == 0 || summary == 0 ||
        image_bytes != SELINOS_STATIC_ELF_RX_LOAD_M0_IMAGE_BYTES) {
        return SELINOS_ELFRT_E_ARGUMENT;
    }
    for (index = 0u; index < image_bytes; ++index) {
        if (image[index] != static_elf_rx_fixture[index]) {
            return SELINOS_ELFRT_E_POLICY;
        }
    }
    if (selinos_elfrt_parse_image(image, image_bytes, 0, summary) !=
            SELINOS_ELFRT_OK ||
        selinos_elfrt_validate_initial_load_policy(summary) != SELINOS_ELFRT_OK ||
        summary->elf_type != 2u || summary->elf_machine != 62u ||
        summary->entry != SELINOS_STATIC_ELF_RX_LOAD_M0_FIXED_ENTRY_VADDR ||
        summary->load_segments != 1u || summary->writable_load_segments != 0u ||
        summary->executable_load_segments != 1u ||
        summary->writable_executable_load_segments != 0u ||
        summary->needed_count != 0u || summary->has_dynamic != 0u ||
        summary->has_interp != 0u || summary->has_rela != 0u ||
        summary->has_textrel != 0u ||
        image[SELINOS_STATIC_ELF_RX_LOAD_M0_LOAD_OFFSET] !=
            SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE0 ||
        image[SELINOS_STATIC_ELF_RX_LOAD_M0_LOAD_OFFSET + 1u] !=
            SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE1) {
        return SELINOS_ELFRT_E_POLICY;
    }
    return SELINOS_ELFRT_OK;
}
