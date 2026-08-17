// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>

#include <allocman/vka.h>
#include <vspace/vspace.h>

/* Runs the paired fixed NX-fault and executable-control mapping witnesses.
 * This is a default-OFF root-only proof, not an ELF loader interface. */
bool selinos_run_x86_nx_mapping_probe(vka_t *root_vka, vspace_t *root_vspace);
