// SPDX-License-Identifier: MIT
#ifndef SELINOS_STATIC_ELF_RX_LOAD_M0_H
#define SELINOS_STATIC_ELF_RX_LOAD_M0_H

#include <stdbool.h>
#include <vka/vka.h>
#include <sel4utils/vspace.h>

/* Performs one bounded parse -> root-private copy -> target RX map -> UD2
 * witness transaction. It never replies, repairs, retries, or resumes twice. */
bool selinos_static_elf_rx_load_m0_start(vka_t *vka, vspace_t *vspace);

#endif
