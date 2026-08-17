// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M1_H
#define SELINOS_ROOT_CONSTRUCT_M1_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Phase 35 M1 is an opt-in, root-only construction proof.  The caller owns
 * the persistent root VKA and loader VSpace.  No non-root code receives either
 * descriptor, a child capability, or a constructor callback. */
bool selinos_root_construct_m1_start_bundle(vka_t *root_vka,
                                            vspace_t *root_vspace);

/* Blocks only on the dedicated construction endpoint.  It accepts at most one
 * fixed request, constructs one fixed inert child while retaining all root
 * authority, then rejects every later or malformed request without reuse. */
void selinos_root_construct_m1_dispatch_loop(void);

#endif
