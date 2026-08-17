// SPDX-License-Identifier: MIT
#ifndef SELINOS_MEMD_M1_PROTOCOL_H
#define SELINOS_MEMD_M1_PROTOCOL_H

#include <sel4/sel4.h>

/* Phase 29 M1 models only a fixed mapping-plan reservation state. */
#define SELINOS_MEMD_M1_RESERVE_FIXED_PLAN 0x4d454d4d31524553ull /* "MEMM1RES" */
#define SELINOS_MEMD_M1_RESERVED 0x4d454d4d31524544ull /* "MEMM1RED" */
#define SELINOS_MEMD_M1_EBUSY 0x4d454d4d31425553ull /* "MEMM1BUS" */
#define SELINOS_MEMD_M1_EINVAL 0x4d454d4d31494e56ull /* "MEMM1INV" */
#define SELINOS_MEMD_M1_SLOT 1u

#define SELINOS_MEMD_M1_REQUEST_WORDS 2u
#define SELINOS_MEMD_M1_REPLY_WORDS 1u

/* sel4utils reserves slots 0-7 in each configured process CSpace. */
#define SELINOS_MEMD_M1_ENDPOINT_SLOT 8u
#define SELINOS_MEMD_M1_PROBE_ENDPOINT_SLOT 8u

#endif
