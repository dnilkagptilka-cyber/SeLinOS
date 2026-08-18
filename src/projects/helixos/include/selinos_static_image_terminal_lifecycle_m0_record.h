// SPDX-License-Identifier: MIT
#ifndef SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_RECORD_H
#define SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_RECORD_H

#include <sel4/sel4.h>

#include "selinos_static_image_terminal_lifecycle_m0_protocol.h"

struct selinos_static_image_terminal_lifecycle_m0_record {
    seL4_Word terminal_ip;
    seL4_Word terminal_vector;
    seL4_Word tcb_observed;
    seL4_Word cnode_observed;
    seL4_Word vspace_observed;
    seL4_Word entry_frame_observed;
    seL4_Word stack_frame_observed;
    seL4_Word ipc_frame_observed;
};

#endif
