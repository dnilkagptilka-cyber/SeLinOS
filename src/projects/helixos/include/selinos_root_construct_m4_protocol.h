// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M4_PROTOCOL_H
#define SELINOS_ROOT_CONSTRUCT_M4_PROTOCOL_H

#include <sel4/sel4.h>

#include "selinos_root_construct_m2_protocol.h"

/* Phase 38 M4: one root-created endpoint is copied at fixed slots before the
 * root-selected child is spawned. No image, stack, register, TLS, clone,
 * scheduler, credential, VFS or device parameter appears on this path. */
#define SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE 0x52434d34444f4e45ull /* "RCM4DONE" */
#define SELINOS_ROOT_CONSTRUCT_M4_SLOT 1u
#define SELINOS_ROOT_CONSTRUCT_M4_GENERATION 1u
#define SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS 3u

#define SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT 8u
#define SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT 9u
#define SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT 10u

#endif
