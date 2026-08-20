// SPDX-License-Identifier: MIT
#include <allocman/vka.h>
#include <sel4/sel4.h>
#include <sel4utils/mapping.h>
#include <sel4utils/vspace.h>
#include <stdint.h>

#include "selinos_static_elf_two_rx_ro_rw_bss_m0.h"
#include "selinos_static_elf_two_rx_ro_rw_bss_m0_fixture.h"
#include "selinos_static_elf_two_rx_ro_rw_bss_m0_protocol.h"

bool selinos_static_elf_two_rx_ro_rw_bss_m0_start(vka_t *vka, vspace_t *vspace)
{
    vka_object_t fault_endpoint;
    vka_object_t target_tcb;
    vka_object_t target_cnode;
    vka_object_t target_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_second_rx_frame;
    vka_object_t target_stack_frame;
    vka_object_t target_data_frame;
    vka_object_t target_ro_frame;
    vka_object_t paging_objects[8];
    struct selinos_elfrt_summary summary;
    const selinos_elfrt_u8 *image;
    selinos_elfrt_size_t image_bytes = 0u;
    seL4_UserContext requested_context = {0};
    seL4_UserContext observed_context = {0};
    seL4_MessageInfo_t fault_message;
    const seL4_CapRights_t read_only = seL4_CapRights_new(0, 0, 1, 0);
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    seL4_Word fault_badge = 0u;
    seL4_Word index;
    int paging_object_count = 0;
    void *root_entry_mapping = NULL;
    void *root_second_rx_mapping = NULL;
    void *root_ro_mapping = NULL;
    void *root_data_mapping = NULL;

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                   "Phase 82 requires complete pinned x86_64 context");
    if (vka == NULL || vspace == NULL) {
        return false;
    }
    image = selinos_static_elf_two_rx_ro_rw_bss_m0_fixture(&image_bytes);
    if (selinos_static_elf_two_rx_ro_rw_bss_m0_validate_fixture(image, image_bytes,
                                                                  &summary) != SELINOS_ELFRT_OK ||
        summary.entry != SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_ENTRY_VADDR) {
        return false;
    }
    if (vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &target_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka,
                               SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                               &target_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &target_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_second_rx_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_data_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ro_frame) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_SECOND_RX_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_second_rx_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_DATA_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_data_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_RO_FRAME_SLOT,
                        SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ro_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 target_vspace_root.cptr) != seL4_NoError) {
        return false;
    }

    root_entry_mapping = vspace_map_pages(vspace, &target_entry_frame.cptr, NULL,
                                          read_write, 1u, seL4_PageBits, 1u);
    if (root_entry_mapping == NULL) {
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_entry_mapping)[index] = 0u;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_ENTRY_PAYLOAD_BYTES;
         ++index) {
        ((volatile uint8_t *)root_entry_mapping)[index] =
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_ENTRY_LOAD_OFFSET + index];
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_ENTRY_PAYLOAD_BYTES;
         ++index) {
        if (((volatile uint8_t *)root_entry_mapping)[index] !=
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_ENTRY_LOAD_OFFSET + index]) {
            return false;
        }
    }
    vspace_unmap_pages(vspace, root_entry_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_entry_mapping = NULL;

    root_second_rx_mapping = vspace_map_pages(vspace, &target_second_rx_frame.cptr, NULL,
                                               read_write, 1u, seL4_PageBits, 1u);
    if (root_second_rx_mapping == NULL) {
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_second_rx_mapping)[index] = 0u;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_SECOND_RX_PAYLOAD_BYTES;
         ++index) {
        ((volatile uint8_t *)root_second_rx_mapping)[index] =
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_SECOND_RX_LOAD_OFFSET + index];
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_SECOND_RX_PAYLOAD_BYTES;
         ++index) {
        if (((volatile uint8_t *)root_second_rx_mapping)[index] !=
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_SECOND_RX_LOAD_OFFSET + index]) {
            return false;
        }
    }
    vspace_unmap_pages(vspace, root_second_rx_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_second_rx_mapping = NULL;

    root_ro_mapping = vspace_map_pages(vspace, &target_ro_frame.cptr, NULL,
                                       read_write, 1u, seL4_PageBits, 1u);
    if (root_ro_mapping == NULL) {
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_ro_mapping)[index] = 0u;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_RO_INITIALIZED_BYTES;
         ++index) {
        ((volatile uint8_t *)root_ro_mapping)[index] =
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + index];
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_RO_INITIALIZED_BYTES;
         ++index) {
        if (((volatile uint8_t *)root_ro_mapping)[index] !=
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_RO_LOAD_OFFSET + index]) {
            return false;
        }
    }
    vspace_unmap_pages(vspace, root_ro_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_ro_mapping = NULL;

    root_data_mapping = vspace_map_pages(vspace, &target_data_frame.cptr, NULL,
                                         read_write, 1u, seL4_PageBits, 1u);
    if (root_data_mapping == NULL) {
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_data_mapping)[index] = 0u;
    }
    for (index = 0u;
         index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_DATA_INITIALIZED_BYTES; ++index) {
        ((volatile uint8_t *)root_data_mapping)[index] =
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + index];
    }
    for (index = 0u;
         index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_DATA_INITIALIZED_BYTES; ++index) {
        if (((volatile uint8_t *)root_data_mapping)[index] !=
            image[SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_DATA_LOAD_OFFSET + index]) {
            return false;
        }
    }
    for (index = SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_BSS_OFFSET;
         index < SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_DATA_MEMORY_BYTES; ++index) {
        if (((volatile uint8_t *)root_data_mapping)[index] != 0u) {
            return false;
        }
    }
    vspace_unmap_pages(vspace, root_data_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_data_mapping = NULL;

    if (sel4utils_map_page(vka, target_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_stack_frame.cptr,
            (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_STACK_VADDR,
            read_write,
            (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                     seL4_X86_ExecuteDisable),
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_entry_frame.cptr,
            (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_ENTRY_VADDR,
            read_only, seL4_X86_Default_VMAttributes,
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_second_rx_frame.cptr,
            (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_SECOND_RX_VADDR,
            read_only, seL4_X86_Default_VMAttributes,
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_ro_frame.cptr,
            (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_RO_VADDR,
            read_only,
            (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                     seL4_X86_ExecuteDisable),
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_data_frame.cptr,
            (void *)SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_DATA_VADDR,
            read_write,
            (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                     seL4_X86_ExecuteDisable),
            paging_objects, &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(target_tcb.cptr,
                           SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                           target_cnode.cptr, 0u, target_vspace_root.cptr, 0u,
                           SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_IPC_BUFFER_VADDR,
                           target_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }

    requested_context.rip = SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(target_tcb.cptr, 0u, 0u,
                                SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(target_tcb.cptr, 0u, 0u,
                               SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError ||
        observed_context.rip != requested_context.rip ||
        observed_context.rsp != requested_context.rsp ||
        ((const seL4_Word *)&observed_context)
            [SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_X86_64_RFLAGS_WORD] !=
                SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_X86_64_NORMALIZED_RFLAGS ||
        seL4_TCB_Resume(target_tcb.cptr) != seL4_NoError) {
        return false;
    }

    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_UserException ||
        fault_badge != SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_SECOND_RX_VADDR +
                SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_SUCCESS_UD2_OFFSET ||
        seL4_GetMR(seL4_UserException_SP) !=
            SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_FIXED_STACK_POINTER ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_STATIC_ELF_TWO_RX_RO_RW_BSS_M0_INVALID_OPCODE_VECTOR) {
        return false;
    }
    return true;
}
