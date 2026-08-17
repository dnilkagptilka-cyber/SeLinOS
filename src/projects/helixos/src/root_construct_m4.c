// SPDX-License-Identifier: MIT
#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4/sel4.h>
#include <sel4utils/process.h>

#include "selinos_root_construct_m4.h"
#include "selinos_root_construct_m4_protocol.h"

/* This endpoint is root-owned. The two process-local cap copies are used only
 * by the closed M4 witness trace; no VKA, CNode or endpoint allocator cap is
 * transferred. */
static vka_object_t root_construct_m4_completion_endpoint;
static bool root_construct_m4_endpoint_allocated;
static bool root_construct_m4_taskd_prepared;
static bool root_construct_m4_child_prepared;

bool selinos_root_construct_m4_prepare_taskd(sel4utils_process_t *taskd,
                                             vka_t *root_vka)
{
    if (taskd == NULL || root_vka == NULL || root_construct_m4_taskd_prepared ||
        vka_alloc_endpoint(root_vka, &root_construct_m4_completion_endpoint) != seL4_NoError) {
        return false;
    }
    root_construct_m4_endpoint_allocated = true;
    const seL4_CPtr taskd_slot = sel4utils_copy_cap_to_process(
        taskd, root_vka, root_construct_m4_completion_endpoint.cptr);
    if (taskd_slot != SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT) {
        return false;
    }
    root_construct_m4_taskd_prepared = true;
    return true;
}

bool selinos_root_construct_m4_prepare_child(sel4utils_process_t *child,
                                             vka_t *root_vka)
{
    if (child == NULL || root_vka == NULL || !root_construct_m4_endpoint_allocated ||
        !root_construct_m4_taskd_prepared || root_construct_m4_child_prepared) {
        return false;
    }
    const seL4_CPtr child_slot = sel4utils_copy_cap_to_process(
        child, root_vka, root_construct_m4_completion_endpoint.cptr);
    if (child_slot != SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT) {
        return false;
    }
    root_construct_m4_child_prepared = true;
    return true;
}
