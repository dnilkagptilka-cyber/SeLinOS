// SPDX-License-Identifier: MIT
#include <allocman/vka.h>
#include <sel4/sel4.h>
#include <sel4utils/mapping.h>
#include <sel4utils/vspace.h>
#include <stdint.h>

#include "selinos_static_elf_rx_rw_bss_m0.h"
#include "selinos_static_elf_rx_rw_bss_m0_fixture.h"
#include "selinos_static_elf_rx_rw_bss_m0_protocol.h"

static void phase80_debug_puts(const char *text)
{
    while (*text != '\0') {
        seL4_DebugPutChar(*text++);
    }
}

static void phase80_debug_hex(seL4_Word value)
{
    static const char hex[] = "0123456789abcdef";
    seL4_Word shift = (seL4_WordBits - 4u);

    phase80_debug_puts("0x");
    for (;;) {
        seL4_DebugPutChar(hex[(value >> shift) & 0xfu]);
        if (shift == 0u) {
            break;
        }
        shift -= 4u;
    }
}

bool selinos_static_elf_rx_rw_bss_m0_start(vka_t *vka, vspace_t *vspace)
{
    vka_object_t fault_endpoint;
    vka_object_t target_tcb;
    vka_object_t target_cnode;
    vka_object_t target_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_stack_frame;
    vka_object_t target_data_frame;
    vka_object_t paging_objects[6];
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
    void *root_data_mapping = NULL;

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_STATIC_ELF_RX_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                   "Phase 80 requires complete pinned x86_64 context");
    if (vka == NULL || vspace == NULL) {
        return false;
    }
    image = selinos_static_elf_rx_rw_bss_m0_fixture(&image_bytes);
    if (selinos_static_elf_rx_rw_bss_m0_validate_fixture(image, image_bytes,
                                                          &summary) != SELINOS_ELFRT_OK ||
        summary.entry != SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_ENTRY_VADDR) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: fixture validation gate rejected.\n");
        return false;
    }
    if (vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &target_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka,
                               SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                               &target_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &target_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_data_frame) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_DATA_FRAME_SLOT,
                        SELINOS_STATIC_ELF_RX_RW_BSS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_data_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 target_vspace_root.cptr) != seL4_NoError) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: allocation or target-capability setup rejected.\n");
        return false;
    }

    root_entry_mapping = vspace_map_pages(vspace, &target_entry_frame.cptr, NULL,
                                          read_write, 1u, seL4_PageBits, 1u);
    if (root_entry_mapping == NULL) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: root text alias map rejected.\n");
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_entry_mapping)[index] = 0u;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_TEXT_PAYLOAD_BYTES;
         ++index) {
        ((volatile uint8_t *)root_entry_mapping)[index] =
            image[SELINOS_STATIC_ELF_RX_RW_BSS_M0_TEXT_LOAD_OFFSET + index];
    }
    vspace_unmap_pages(vspace, root_entry_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_entry_mapping = NULL;

    root_data_mapping = vspace_map_pages(vspace, &target_data_frame.cptr, NULL,
                                         read_write, 1u, seL4_PageBits, 1u);
    if (root_data_mapping == NULL) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: root data alias map rejected.\n");
        return false;
    }
    for (index = 0u; index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_PAGE_BYTES; ++index) {
        ((volatile uint8_t *)root_data_mapping)[index] = 0u;
    }
    for (index = 0u;
         index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_DATA_INITIALIZED_BYTES; ++index) {
        ((volatile uint8_t *)root_data_mapping)[index] =
            image[SELINOS_STATIC_ELF_RX_RW_BSS_M0_DATA_LOAD_OFFSET + index];
    }
    for (index = 0u;
         index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_DATA_INITIALIZED_BYTES; ++index) {
        if (((volatile uint8_t *)root_data_mapping)[index] !=
            image[SELINOS_STATIC_ELF_RX_RW_BSS_M0_DATA_LOAD_OFFSET + index]) {
            phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: initialized data copy mismatch.\n");
            return false;
        }
    }
    for (index = SELINOS_STATIC_ELF_RX_RW_BSS_M0_BSS_OFFSET;
         index < SELINOS_STATIC_ELF_RX_RW_BSS_M0_DATA_MEMORY_BYTES; ++index) {
        if (((volatile uint8_t *)root_data_mapping)[index] != 0u) {
            phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: BSS zero initialization mismatch.\n");
            return false;
        }
    }
    vspace_unmap_pages(vspace, root_data_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_data_mapping = NULL;

    if (sel4utils_map_page(vka, target_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_stack_frame.cptr,
            (void *)SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_STACK_VADDR,
            read_write,
            (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                     seL4_X86_ExecuteDisable),
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_entry_frame.cptr,
            (void *)SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_ENTRY_VADDR,
            read_only, seL4_X86_Default_VMAttributes,
            paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(
            vka, target_vspace_root.cptr, target_data_frame.cptr,
            (void *)SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_DATA_VADDR,
            read_write,
            (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                     seL4_X86_ExecuteDisable),
            paging_objects, &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(target_tcb.cptr,
                           SELINOS_STATIC_ELF_RX_RW_BSS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                           target_cnode.cptr, 0u, target_vspace_root.cptr, 0u,
                           SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_IPC_BUFFER_VADDR,
                           target_ipc_frame.cptr) != seL4_NoError) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: target W^X mapping or TCB configuration rejected.\n");
        return false;
    }

    requested_context.rip = SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(target_tcb.cptr, 0u, 0u,
                                SELINOS_STATIC_ELF_RX_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(target_tcb.cptr, 0u, 0u,
                               SELINOS_STATIC_ELF_RX_RW_BSS_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError ||
        observed_context.rip != requested_context.rip ||
        observed_context.rsp != requested_context.rsp ||
        ((const seL4_Word *)&observed_context)
            [SELINOS_STATIC_ELF_RX_RW_BSS_M0_X86_64_RFLAGS_WORD] !=
                SELINOS_STATIC_ELF_RX_RW_BSS_M0_X86_64_NORMALIZED_RFLAGS ||
        seL4_TCB_Resume(target_tcb.cptr) != seL4_NoError) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: target register setup or single resume rejected.\n");
        return false;
    }

    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_UserException) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: terminal label mismatch ");
        phase80_debug_hex(seL4_MessageInfo_get_label(fault_message));
        if (seL4_MessageInfo_get_label(fault_message) == seL4_Fault_VMFault) {
            phase80_debug_puts(" ip=");
            phase80_debug_hex(seL4_GetMR(seL4_VMFault_IP));
            phase80_debug_puts(" addr=");
            phase80_debug_hex(seL4_GetMR(seL4_VMFault_Addr));
            phase80_debug_puts(" prefetch=");
            phase80_debug_hex(seL4_GetMR(seL4_VMFault_PrefetchFault));
            phase80_debug_puts(" fsr=");
            phase80_debug_hex(seL4_GetMR(seL4_VMFault_FSR));
        }
        phase80_debug_puts(".\n");
        return false;
    }
    if (fault_badge != SELINOS_STATIC_ELF_RX_RW_BSS_M0_FAULT_BADGE) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: terminal badge mismatch.\n");
        return false;
    }
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
        SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_ENTRY_VADDR +
            SELINOS_STATIC_ELF_RX_RW_BSS_M0_SUCCESS_UD2_OFFSET) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: terminal fault IP mismatch.\n");
        return false;
    }
    if (seL4_GetMR(seL4_UserException_SP) !=
        SELINOS_STATIC_ELF_RX_RW_BSS_M0_FIXED_STACK_POINTER) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: terminal stack pointer mismatch.\n");
        return false;
    }
    if (seL4_GetMR(seL4_UserException_Number) !=
        SELINOS_STATIC_ELF_RX_RW_BSS_M0_INVALID_OPCODE_VECTOR) {
        phase80_debug_puts("SeLinOS static ELF RX/RW+BSS M0: terminal vector mismatch.\n");
        return false;
    }
    return true;
}
