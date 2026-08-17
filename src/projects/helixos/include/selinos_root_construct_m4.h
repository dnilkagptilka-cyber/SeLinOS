// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M4_H
#define SELINOS_ROOT_CONSTRUCT_M4_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/process.h>

/* Root allocates one completion endpoint and copies its receive cap only to the
 * selected M4 taskd client. No VKA or endpoint authority crosses this API. */
bool selinos_root_construct_m4_prepare_taskd(sel4utils_process_t *taskd,
                                             vka_t *root_vka);

/* Root copies the matching send cap only into the root-selected M4 child before
 * the child is spawned suspended. */
bool selinos_root_construct_m4_prepare_child(sel4utils_process_t *child,
                                             vka_t *root_vka);

#endif
