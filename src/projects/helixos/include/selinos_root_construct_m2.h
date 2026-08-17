// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M2_H
#define SELINOS_ROOT_CONSTRUCT_M2_H

#include <stdbool.h>

#include <sel4/sel4.h>

/* Root calls this once after successful M1 construction. It retains the source
 * cap; M2 may copy it at most once through the same root endpoint. */
bool selinos_root_construct_m2_record_child_tcb(seL4_CPtr child_tcb);

/* Root dispatches the closed M2 lease request after M1 reached ROOT_CONSTRUCTED.
 * The reply has either exactly one cap or no cap; this function performs no
 * construction, start, resume, suspension, teardown or reuse. */
bool selinos_root_construct_m2_try_dispatch(seL4_Word badge,
                                            seL4_MessageInfo_t message);

#endif
