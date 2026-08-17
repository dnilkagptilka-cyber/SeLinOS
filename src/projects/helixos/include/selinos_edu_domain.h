// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>
#include <vka/vka.h>
#include <vspace/vspace.h>

#include "selinos_pci.h"

/*
 * Construct the driver VSpace/CSpace, copy only BAR0 device frame caps, map
 * them at the fixed driver ABI address and resume the self-authored edu probe.
 * IRQ/DMA are intentionally not delegated in this M0 stage.
 */
#define SELINOS_MAX_QEMU_EDU_INSTANCES 2u

/* The bounded M3 profile creates an independent least-authority service and
 * driver-domain bundle per enumerated QEMU edu function. `instance_index` is
 * root-private bookkeeping, never an identifier trusted from the driver. */
bool selinos_start_deviced_domain(vka_t *vka, vspace_t *root_vspace,
                                  const struct selinos_qemu_edu_resource *resource,
                                  unsigned int instance_index);

bool selinos_start_edu_driver_domain(vka_t *vka, vspace_t *root_vspace,
                                     const struct selinos_qemu_edu_resource *resource,
                                     unsigned int instance_index);
