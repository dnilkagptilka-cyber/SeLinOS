// SPDX-License-Identifier: MIT
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <allocman/vka.h>
#include <vka/capops.h>
#include <sel4/sel4.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>
#include <vspace/vspace.h>

#include "selinos_x86_nx_probe.h"
#include "selinos_x86_nx_probe_protocol.h"

static bool write_fixed_ret_opcode(vspace_t *root_vspace, vka_object_t *frame,
                                   void **root_mapping)
{
    void *mapping = vspace_map_pages(root_vspace, &frame->cptr, NULL,
                                     seL4_AllRights, 1u, seL4_PageBits, 1);
    if (mapping == NULL || root_mapping == NULL) {
        return false;
    }
    ((volatile uint8_t *)mapping)[0] = SELINOS_X86_NX_PROBE_RET_OPCODE;
    *root_mapping = mapping;
    return true;
}

static bool map_fixed_test_page(sel4utils_process_t *child, vka_t *root_vka,
                                seL4_CPtr frame_cap, bool execute_disable)
{
    vka_object_t page_table_objects[3];
    int object_count = 0;
    const seL4_X86_VMAttributes attributes =
        execute_disable
            ? (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                      seL4_X86_ExecuteDisable)
            : seL4_X86_Default_VMAttributes;

    return sel4utils_map_page_with_attributes(root_vka, child->pd.cptr,
                                               frame_cap,
                                               (void *)SELINOS_X86_NX_PROBE_VADDR,
                                               seL4_AllRights, attributes,
                                               page_table_objects,
                                               &object_count) == seL4_NoError;
}

bool selinos_run_x86_nx_mapping_probe(vka_t *root_vka, vspace_t *root_vspace)
{
    sel4utils_process_t child;
    vka_object_t endpoint = {0};
    vka_object_t test_frame = {0};
    cspacepath_t child_frame_path = {0};
    cspacepath_t root_frame_path;
    void *root_mapping = NULL;
    char *argv[] = {"selinos-x86-nx-mapping-probe-child", NULL};
    seL4_Word badge = 0u;
    seL4_MessageInfo_t message;
    bool child_frame_copied = false;
    bool child_mapping_present = false;
    bool success = false;

    if (root_vka == NULL || root_vspace == NULL ||
        sel4utils_configure_process(&child, root_vka, root_vspace,
                                    "selinos-x86-nx-mapping-probe-child") != 0 ||
        vka_alloc_endpoint(root_vka, &endpoint) != seL4_NoError ||
        sel4utils_copy_cap_to_process(&child, root_vka, endpoint.cptr) !=
            SELINOS_X86_NX_PROBE_SUCCESS_ENDPOINT_SLOT ||
        sel4utils_spawn_process_v(&child, root_vka, root_vspace,
                                  1, argv, 0) != 0 ||
        child.fault_endpoint.cptr == seL4_CapNull ||
        vka_alloc_frame(root_vka, seL4_PageBits, &test_frame) != 0 ||
        vka_cspace_alloc_path(root_vka, &child_frame_path) != seL4_NoError) {
        goto out;
    }

    /* Copy while the root source cap has no mapping metadata, then retain a
     * separate root-only alias only to initialize the one-byte test program. */
    vka_cspace_make_path(root_vka, test_frame.cptr, &root_frame_path);
    if (vka_cnode_copy(&child_frame_path, &root_frame_path, seL4_AllRights) !=
            seL4_NoError) {
        goto out;
    }
    child_frame_copied = true;
    if (!map_fixed_test_page(&child, root_vka, child_frame_path.capPtr, true) ||
        !write_fixed_ret_opcode(root_vspace, &test_frame, &root_mapping) ||
        seL4_TCB_Resume(child.thread.tcb.cptr) != seL4_NoError) {
        goto out;
    }
    child_mapping_present = true;

    message = seL4_Recv(child.fault_endpoint.cptr, &badge);
    if (badge != 0u || seL4_MessageInfo_get_label(message) != seL4_Fault_VMFault ||
        seL4_GetMR(seL4_VMFault_IP) != SELINOS_X86_NX_PROBE_VADDR ||
        seL4_GetMR(seL4_VMFault_Addr) != SELINOS_X86_NX_PROBE_VADDR ||
        seL4_GetMR(seL4_VMFault_PrefetchFault) != seL4_InstructionFault) {
        seL4_DebugPutString("SeLinOS W^X NX: execute-disabled fault witness failed.\n");
        goto out;
    }
    seL4_DebugPutString("SeLinOS W^X NX: execute-disabled child produced fixed instruction fault.\n");

    if (root_mapping != NULL) {
        vspace_unmap_pages(root_vspace, root_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
        root_mapping = NULL;
    }
    if (seL4_X86_Page_Unmap(child_frame_path.capPtr) != seL4_NoError ||
        !map_fixed_test_page(&child, root_vka, child_frame_path.capPtr, false)) {
        seL4_DebugPutString("SeLinOS W^X NX: executable remap failed.\n");
        goto out;
    }
    /* Reply only after the executable control mapping replaces the NX leaf. */
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u));
    message = seL4_Recv(endpoint.cptr, &badge);
    if (badge != 0u || seL4_MessageInfo_get_label(message) != 0u ||
        seL4_MessageInfo_get_length(message) !=
            SELINOS_X86_NX_PROBE_SUCCESS_WORDS ||
        seL4_MessageInfo_get_extraCaps(message) != 0u ||
        seL4_GetMR(0) != SELINOS_X86_NX_PROBE_SUCCESS_MAGIC) {
        seL4_DebugPutString("SeLinOS W^X NX: executable control witness failed.\n");
        goto out;
    }
    seL4_DebugPutString("SeLinOS W^X NX: executable control returned fixed witness.\n");
    seL4_DebugPutString("SeLinOS W^X NX: paired fixed mapping proof passed; no ELF execution.\n");
    success = true;

out:
    (void)seL4_TCB_Suspend(child.thread.tcb.cptr);
    if (child_mapping_present) {
        (void)seL4_X86_Page_Unmap(child_frame_path.capPtr);
    }
    if (child_frame_copied) {
        (void)vka_cnode_delete(&child_frame_path);
        vka_cspace_free(root_vka, child_frame_path.capPtr);
    }
    if (root_mapping != NULL) {
        vspace_unmap_pages(root_vspace, root_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (test_frame.cptr != seL4_CapNull) {
        vka_free_object(root_vka, &test_frame);
    }
    if (endpoint.cptr != seL4_CapNull) {
        vka_free_object(root_vka, &endpoint);
    }
    return success;
}
