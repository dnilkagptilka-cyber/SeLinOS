// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M3_PROTOCOL_H
#define SELINOS_ROOT_CONSTRUCT_M3_PROTOCOL_H

#include <sel4/sel4.h>

#include "selinos_root_construct_m2_protocol.h"

/* Phase 37 M3 performs no additional IPC after M1/M2. The root-selected taskd
 * client may invoke exactly this one direct action on the single M2-received
 * cap in the declared slot. It supplies no image, stack, register, TLS, clone,
 * scheduler, credential, VFS or device parameter. */
#define SELINOS_ROOT_CONSTRUCT_M3_CHILD_TCB_SLOT \
    SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT
#define SELINOS_ROOT_CONSTRUCT_M3_RESUME_COUNT 1u

#endif
