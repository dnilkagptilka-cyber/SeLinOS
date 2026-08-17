// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M2_PROTOCOL_H
#define SELINOS_ROOT_CONSTRUCT_M2_PROTOCOL_H

#include <sel4/sel4.h>

/* Phase 36 M2: one provenance-bound control-handle lease after the exact M1
 * construction. No request word is a pointer, image, stack, register, TLS,
 * clone, scheduler, credential, VFS or device parameter. */
#define SELINOS_ROOT_CONSTRUCT_M2_LEASE_TCB 0x52434d324c454153ull /* "RCM2LEAS" */
#define SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED 0x52434d324752414eull /* "RCM2GRAN" */
#define SELINOS_ROOT_CONSTRUCT_M2_REJECTED 0x52434d3252454a43ull /* "RCM2REJC" */

#define SELINOS_ROOT_CONSTRUCT_M2_SLOT 1u
#define SELINOS_ROOT_CONSTRUCT_M2_GENERATION 1u
#define SELINOS_ROOT_CONSTRUCT_M2_REQUEST_WORDS 3u
#define SELINOS_ROOT_CONSTRUCT_M2_REPLY_WORDS 3u
#define SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS 1u
#define SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS 0u

/* The same isolated taskd adapter has the one root endpoint in slot 8. It may
 * receive the one opaque handle only in slot 9 of its own CSpace. */
#define SELINOS_ROOT_CONSTRUCT_M2_TASKD_REQUEST_ENDPOINT_SLOT 8u
#define SELINOS_ROOT_CONSTRUCT_M2_TASKD_CNODE_SLOT 1u
#define SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT 9u

#endif
