// SPDX-License-Identifier: MIT
#ifndef SELINOS_STATIC_ELF_TWO_RX_NOTE_M0_H
#define SELINOS_STATIC_ELF_TWO_RX_NOTE_M0_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Performs one bounded parse -> root-private entry-RX/second-RX/RO/RW copies
 * -> target RX/RX/RO+NX/RW+NX mappings -> one resume -> terminal second-RX
 * UD2 transaction. It never replies, repairs, retries, resumes twice or maps
 * writable text. */
bool selinos_static_elf_two_rx_note_m0_start(vka_t *vka, vspace_t *vspace);

#endif
