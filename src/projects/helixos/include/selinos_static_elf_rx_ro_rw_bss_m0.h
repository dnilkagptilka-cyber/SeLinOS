// SPDX-License-Identifier: MIT
#ifndef SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_H
#define SELINOS_STATIC_ELF_RX_RO_RW_BSS_M0_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Performs one bounded parse -> root-private text/RO/RW copies -> target RX,
 * RO+NX and RW+NX mappings -> one resume -> terminal UD2 transaction. It
 * never replies, repairs, retries, resumes twice or maps writable text. */
bool selinos_static_elf_rx_ro_rw_bss_m0_start(vka_t *vka, vspace_t *vspace);

#endif
