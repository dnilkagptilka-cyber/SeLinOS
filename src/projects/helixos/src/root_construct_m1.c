// SPDX-License-Identifier: MIT
#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4/sel4.h>
#include <selinos-root/gen_config.h>
#include <sel4utils/process.h>
#include <sel4utils/vspace.h>

#include "selinos_root_construct_m1.h"
#include "selinos_root_construct_m1_protocol.h"
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE
#include "selinos_root_construct_m2.h"
#endif
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
#include "selinos_root_construct_m4.h"
#endif

/* This translation unit is linked only into root. The persistent VKA and loader
 * VSpace are accepted from root bootstrap and never copied into any CSpace. */
enum selinos_root_construct_m1_state {
    SELINOS_ROOT_CONSTRUCT_M1_FREE = 0,
    SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTING,
    SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED,
    SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED,
};

static vka_t *root_construct_m1_vka;
static vspace_t *root_construct_m1_vspace;
static seL4_CPtr root_construct_m1_endpoint = seL4_CapNull;
static enum selinos_root_construct_m1_state root_construct_m1_state =
    SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED;

static void reply_root_construct_m1(seL4_Word status)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, SELINOS_ROOT_CONSTRUCT_M1_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_CONSTRUCT_M1_GENERATION);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,
                                    SELINOS_ROOT_CONSTRUCT_M1_REPLY_WORDS));
}

static bool construct_root_fixed_child_m1(void)
{
    sel4utils_process_t child;
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
    char *child_argv[] = {"selinos-root-construct-m4-completion-child", NULL};
    const char *child_image = "selinos-root-construct-m4-completion-child";
#elif CONFIG_SELINOS_ROOT_CONSTRUCT_M3_SINGLE_RESUME
    char *child_argv[] = {"selinos-root-construct-m3-witness-child", NULL};
    const char *child_image = "selinos-root-construct-m3-witness-child";
#else
    char *child_argv[] = {"selinos-root-construct-m1-inert-child", NULL};
    const char *child_image = "selinos-root-construct-m1-inert-child";
#endif

    if (root_construct_m1_vka == NULL || root_construct_m1_vspace == NULL ||
        sel4utils_configure_process(&child, root_construct_m1_vka,
                                    root_construct_m1_vspace,
                                    child_image) != 0) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(child.thread.tcb.cptr, (char *)child_image);
#endif
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
    if (!selinos_root_construct_m4_prepare_child(&child, root_construct_m1_vka)) {
        return false;
    }
#endif
    /* A zero resume argument leaves the fixed root-selected inert child
     * suspended. M1 passes no child TCB, CSpace, VSpace, fault or mapping cap. */
    if (sel4utils_spawn_process_v(&child, root_construct_m1_vka,
                                  root_construct_m1_vspace, 1, child_argv, 0) != 0) {
        return false;
    }
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE
    if (!selinos_root_construct_m2_record_child_tcb(child.thread.tcb.cptr)) {
        return false;
    }
#endif
    return true;
}

bool selinos_root_construct_m1_start_bundle(vka_t *root_vka,
                                            vspace_t *root_vspace)
{
    sel4utils_process_t taskd_client;
    vka_object_t endpoint;
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
    char *taskd_client_argv[] = {"selinos-taskd-root-construct-m4", NULL};
    const char *taskd_client_image = "selinos-taskd-root-construct-m4";
#elif CONFIG_SELINOS_ROOT_CONSTRUCT_M3_SINGLE_RESUME
    char *taskd_client_argv[] = {"selinos-taskd-root-construct-m3", NULL};
    const char *taskd_client_image = "selinos-taskd-root-construct-m3";
#elif CONFIG_SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE
    char *taskd_client_argv[] = {"selinos-taskd-root-construct-m2", NULL};
    const char *taskd_client_image = "selinos-taskd-root-construct-m2";
#else
    char *taskd_client_argv[] = {"selinos-taskd-root-construct-m1", NULL};
    const char *taskd_client_image = "selinos-taskd-root-construct-m1";
#endif
    seL4_CPtr taskd_request_slot;

    if (root_vka == NULL || root_vspace == NULL ||
        root_construct_m1_endpoint != seL4_CapNull ||
        root_construct_m1_state != SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED ||
        sel4utils_configure_process(&taskd_client, root_vka, root_vspace,
                                    taskd_client_image) != 0 ||
        vka_alloc_endpoint(root_vka, &endpoint) != seL4_NoError) {
        return false;
    }

    taskd_request_slot = sel4utils_copy_cap_to_process(&taskd_client, root_vka,
                                                        endpoint.cptr);
    if (taskd_request_slot != SELINOS_ROOT_CONSTRUCT_M1_TASKD_REQUEST_ENDPOINT_SLOT) {
        return false;
    }
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
    if (!selinos_root_construct_m4_prepare_taskd(&taskd_client, root_vka)) {
        return false;
    }
#endif
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd_client.thread.tcb.cptr,
                          (char *)taskd_client_image);
#endif

    root_construct_m1_vka = root_vka;
    root_construct_m1_vspace = root_vspace;
    root_construct_m1_endpoint = endpoint.cptr;
    root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_FREE;
    if (sel4utils_spawn_process_v(&taskd_client, root_vka, root_vspace, 1,
                                  taskd_client_argv, 1) != 0) {
        root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED;
        root_construct_m1_endpoint = seL4_CapNull;
        return false;
    }

    seL4_DebugPutString("SeLinOS root construction M1: isolated taskd request endpoint ready.\n");
    return true;
}

void selinos_root_construct_m1_dispatch_loop(void)
{
    for (;;) {
        seL4_Word badge = 0u;
        seL4_MessageInfo_t message = seL4_Recv(root_construct_m1_endpoint, &badge);
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE
        if (selinos_root_construct_m2_try_dispatch(badge, message)) {
            continue;
        }
#endif
        const bool exact_request =
            badge == 0u &&
            seL4_MessageInfo_get_label(message) == 0u &&
            seL4_MessageInfo_get_length(message) ==
                SELINOS_ROOT_CONSTRUCT_M1_REQUEST_WORDS &&
            seL4_MessageInfo_get_extraCaps(message) == 0u &&
            seL4_GetMR(0) == SELINOS_ROOT_CONSTRUCT_M1_REQUEST &&
            seL4_GetMR(1) == SELINOS_ROOT_CONSTRUCT_M1_SLOT &&
            seL4_GetMR(2) == SELINOS_ROOT_CONSTRUCT_M1_GENERATION;

        if (!exact_request || root_construct_m1_state != SELINOS_ROOT_CONSTRUCT_M1_FREE) {
            root_construct_m1_state = root_construct_m1_state ==
                                          SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED
                                      ? SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED
                                      : SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED;
            reply_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED);
            seL4_DebugPutString("SeLinOS root construction M1: malformed or duplicate request rejected; no allocation.\n");
            continue;
        }

        root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTING;
        seL4_DebugPutString("SeLinOS root construction M1: exact fixed request accepted; root constructing.\n");
        if (!construct_root_fixed_child_m1()) {
            root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED;
            reply_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED);
            seL4_DebugPutString("SeLinOS root construction M1: fixed child construction failed; no cap delivered.\n");
            continue;
        }

        root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED;
        reply_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED);
        seL4_DebugPutString("SeLinOS root construction M1: one fixed child constructed and spawned suspended; no cap delivered.\n");
    }
}
