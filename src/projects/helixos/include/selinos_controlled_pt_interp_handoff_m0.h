// SPDX-License-Identifier: MIT
#ifndef SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_H
#define SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Performs one bounded parse -> root-private entry-RX/second-RX/RO/RW copies
 * -> target RX/RX/RO+NX/RW+NX mappings -> one resume -> terminal second-RX
 * UD2 transaction. It never replies, repairs, retries, resumes twice or maps
 * writable text. */
bool selinos_controlled_pt_interp_handoff_m0_start(vka_t *vka, vspace_t *vspace);

#endif
