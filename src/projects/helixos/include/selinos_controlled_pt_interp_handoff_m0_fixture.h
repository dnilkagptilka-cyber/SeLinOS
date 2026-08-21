// SPDX-License-Identifier: MIT
#ifndef SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_FIXTURE_H
#define SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_FIXTURE_H

#include "selinos_elfrt.h"

const selinos_elfrt_u8 *selinos_controlled_pt_interp_handoff_m0_fixture(
    selinos_elfrt_size_t *image_bytes);

/* Accepts only the fixed M0 ELF64 ET_EXEC fixture: a five-byte entry RX jump,
 * a separate second RX execution page, a separate RO page and a separate
 * RW non-executable page with a bounded zero-filled BSS tail. It maps nothing
 * and owns no capability. */
int selinos_controlled_pt_interp_handoff_m0_validate_fixture(
    const selinos_elfrt_u8 *image, selinos_elfrt_size_t image_bytes,
    struct selinos_elfrt_summary *summary);

#endif
