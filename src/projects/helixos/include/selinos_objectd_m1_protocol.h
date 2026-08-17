// SPDX-License-Identifier: MIT
#ifndef SELINOS_OBJECTD_M1_PROTOCOL_H
#define SELINOS_OBJECTD_M1_PROTOCOL_H

#include <sel4/sel4.h>

/* Phase 28 M1 models only a fixed reservation state, not object creation. */
#define SELINOS_OBJECTD_M1_RESERVE 0x4f424a4d31524553ull /* "OBJM1RES" */
#define SELINOS_OBJECTD_M1_RESERVED 0x4f424a4d31524544ull /* "OBJM1RED" */
#define SELINOS_OBJECTD_M1_EBUSY 0x4f424a4d31425553ull /* "OBJM1BUS" */
#define SELINOS_OBJECTD_M1_EINVAL 0x4f424a4d31494e56ull /* "OBJM1INV" */
#define SELINOS_OBJECTD_M1_SLOT 1u

#define SELINOS_OBJECTD_M1_REQUEST_WORDS 2u
#define SELINOS_OBJECTD_M1_REPLY_WORDS 1u

/* sel4utils reserves slots 0-7 in each configured process CSpace. */
#define SELINOS_OBJECTD_M1_ENDPOINT_SLOT 8u
#define SELINOS_OBJECTD_M1_PROBE_ENDPOINT_SLOT 8u

#endif
