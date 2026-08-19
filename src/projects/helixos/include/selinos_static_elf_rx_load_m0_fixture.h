// SPDX-License-Identifier: MIT
#ifndef SELINOS_STATIC_ELF_RX_LOAD_M0_FIXTURE_H
#define SELINOS_STATIC_ELF_RX_LOAD_M0_FIXTURE_H

#include "selinos_elfrt.h"

const selinos_elfrt_u8 *selinos_static_elf_rx_load_m0_fixture(
    selinos_elfrt_size_t *image_bytes);

/* Accepts only the fixed M0 ELF64 ET_EXEC fixture geometry and payload after
 * the parser and no-W+X initial-load policy both pass. It maps nothing and
 * owns no memory or capability. */
int selinos_static_elf_rx_load_m0_validate_fixture(
    const selinos_elfrt_u8 *image, selinos_elfrt_size_t image_bytes,
    struct selinos_elfrt_summary *summary);

#endif
