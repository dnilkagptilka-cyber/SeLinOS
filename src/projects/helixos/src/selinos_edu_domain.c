// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <vka/object.h>
#include <vspace/vspace.h>

#include "selinos_deviced_protocol.h"
#include "selinos_dmad_protocol.h"
#include "selinos_edu_domain.h"
#include "selinos_irqd_protocol.h"

#define SELINOS_EDU_DRIVER_VADDR ((void *)0x600000000000ull)
#define SELINOS_EDU_PAGE_COUNT (SELINOS_QEMU_EDU_BAR0_SIZE >> seL4_PageBits)
#define SELINOS_EDU_CSPACE_BITS 10u
#define SELINOS_EDU_DMA_MASK 0x0ffffffful
#define SELINOS_EDU_DMA_PADDR 0x02000000ul

static vka_object_t edu_bar_frames[SELINOS_MAX_QEMU_EDU_INSTANCES][SELINOS_EDU_PAGE_COUNT];
static seL4_CPtr edu_root_frame_caps[SELINOS_MAX_QEMU_EDU_INSTANCES][SELINOS_EDU_PAGE_COUNT];
static seL4_CPtr edu_driver_frame_caps[SELINOS_MAX_QEMU_EDU_INSTANCES][SELINOS_EDU_PAGE_COUNT];
static vka_object_t edu_irqd_hw_notification[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_irqd_completion_endpoint[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_irqd_request_endpoint[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_driver_notification[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_dmad_free_endpoint[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_deviced_request_endpoint[SELINOS_MAX_QEMU_EDU_INSTANCES];
static vka_object_t edu_dma_frame[SELINOS_MAX_QEMU_EDU_INSTANCES];
static seL4_CPtr edu_irq_handler_cap[SELINOS_MAX_QEMU_EDU_INSTANCES];
static seL4_CPtr edu_driver_register_control_cap[SELINOS_MAX_QEMU_EDU_INSTANCES];
static seL4_CPtr edu_driver_unregister_control_cap[SELINOS_MAX_QEMU_EDU_INSTANCES];
static seL4_CPtr edu_dma_frame_cap[SELINOS_MAX_QEMU_EDU_INSTANCES];
static uintptr_t edu_dma_paddr[SELINOS_MAX_QEMU_EDU_INSTANCES];
static char edu_dma_argument[SELINOS_MAX_QEMU_EDU_INSTANCES][9];
static char edu_irq_argument[SELINOS_MAX_QEMU_EDU_INSTANCES][3];
static char edu_bar_argument[SELINOS_MAX_QEMU_EDU_INSTANCES][17];
static char edu_bar_len_argument[SELINOS_MAX_QEMU_EDU_INSTANCES][17];
static char edu_token_argument[SELINOS_MAX_QEMU_EDU_INSTANCES][17];
static seL4_Word edu_device_token[SELINOS_MAX_QEMU_EDU_INSTANCES];
static seL4_Word next_device_token = 1u;

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

static int valid_instance(unsigned int instance_index)
{
    return instance_index < SELINOS_MAX_QEMU_EDU_INSTANCES;
}

static void format_word_hex(seL4_Word value, char output[17])
{
    static const char hex[] = "0123456789abcdef";
    for (unsigned int i = 0u; i < 16u; ++i) {
        const unsigned int shift = 60u - 4u * i;
        output[i] = hex[(value >> shift) & 0xfu];
    }
    output[16] = '\0';
}

static void format_byte_hex(unsigned int value, char output[3])
{
    static const char hex[] = "0123456789abcdef";
    output[0] = hex[(value >> 4u) & 0xfu];
    output[1] = hex[value & 0xfu];
    output[2] = '\0';
}

static bool mint_edu_control_cap(vka_t *vka, unsigned int instance_index,
                                  seL4_CPtr *destination, seL4_Word badge)
{
    cspacepath_t destination_path;
    cspacepath_t source_path;

    if (!valid_instance(instance_index) || vka_cspace_alloc(vka, destination) != 0) {
        return false;
    }
    vka_cspace_make_path(vka, *destination, &destination_path);
    vka_cspace_make_path(vka, edu_irqd_hw_notification[instance_index].cptr, &source_path);
    return seL4_CNode_Mint(destination_path.root, destination_path.capPtr,
                           destination_path.capDepth, source_path.root,
                           source_path.capPtr, source_path.capDepth,
                           seL4_AllRights, badge) == seL4_NoError;
}

static bool issue_edu_irq(vka_t *vka, unsigned int instance_index,
                          unsigned int irq_line)
{
    if (!valid_instance(instance_index) ||
        vka_alloc_notification(vka, &edu_irqd_hw_notification[instance_index]) != 0 ||
        vka_alloc_endpoint(vka, &edu_irqd_completion_endpoint[instance_index]) != 0 ||
        vka_alloc_endpoint(vka, &edu_irqd_request_endpoint[instance_index]) != 0 ||
        vka_alloc_notification(vka, &edu_driver_notification[instance_index]) != 0 ||
        vka_alloc_endpoint(vka, &edu_dmad_free_endpoint[instance_index]) != 0 ||
        vka_cspace_alloc(vka, &edu_irq_handler_cap[instance_index]) != 0) {
        debug_puts("SeLinOS M3: per-device IRQ resource allocation failed.\n");
        return false;
    }

    if (seL4_IRQControl_GetIOAPIC(seL4_CapIRQControl, seL4_CapInitThreadCNode,
                                  edu_irq_handler_cap[instance_index], seL4_WordBits,
                                  0, irq_line, 1, 1, irq_line) != seL4_NoError ||
        seL4_IRQHandler_SetNotification(edu_irq_handler_cap[instance_index],
                                         edu_irqd_hw_notification[instance_index].cptr) !=
            seL4_NoError) {
        debug_puts("SeLinOS M3: per-device IRQControl grant failed.\n");
        return false;
    }
    return true;
}

static bool issue_edu_dma_lease(vka_t *vka, unsigned int instance_index)
{
    /* Separate per-device frames by 16 MiB so each request stays in a
     * distinct root-visible low-memory allocation region while remaining
     * below EDU's 28-bit DMA ceiling. */
    const uintptr_t requested_paddr = SELINOS_EDU_DMA_PADDR +
                                      ((uintptr_t)instance_index << 24u);
    if (!valid_instance(instance_index) ||
        vka_alloc_frame_at(vka, seL4_PageBits, requested_paddr,
                           &edu_dma_frame[instance_index]) != 0) {
        debug_puts("SeLinOS M3: per-device pinned DMA frame allocation failed.\n");
        return false;
    }
    edu_dma_paddr[instance_index] = vka_object_paddr(vka, &edu_dma_frame[instance_index]);
    if (edu_dma_paddr[instance_index] != requested_paddr ||
        edu_dma_paddr[instance_index] > SELINOS_EDU_DMA_MASK) {
        debug_puts("SeLinOS M3: per-device DMA address mask validation failed.\n");
        vka_free_object(vka, &edu_dma_frame[instance_index]);
        return false;
    }
    for (unsigned int i = 0u; i < 8u; ++i) {
        static const char hex[] = "0123456789abcdef";
        const unsigned int shift = 28u - 4u * i;
        edu_dma_argument[instance_index][i] =
            hex[(edu_dma_paddr[instance_index] >> shift) & 0xfu];
    }
    edu_dma_argument[instance_index][8] = '\0';
    return true;
}

bool selinos_start_deviced_domain(vka_t *vka, vspace_t *root_vspace,
                                  const struct selinos_qemu_edu_resource *resource,
                                  unsigned int instance_index)
{
    sel4utils_process_t deviced;
    char *dormant_argv[] = {"selinos-deviced", NULL};
    char *active_argv[] = {"selinos-deviced", edu_bar_argument[instance_index],
                           edu_bar_len_argument[instance_index],
                           edu_irq_argument[instance_index],
                           edu_token_argument[instance_index], NULL};
    const int has_resource = resource != NULL;

    if (vka == NULL || root_vspace == NULL || !valid_instance(instance_index)) {
        return false;
    }
    if (has_resource) {
        if (resource->bar0_size != SELINOS_QEMU_EDU_BAR0_SIZE ||
            resource->irq_line == 0xffu || resource->bar0_paddr == 0u ||
            vka_alloc_endpoint(vka, &edu_deviced_request_endpoint[instance_index]) != 0) {
            return false;
        }
        if (next_device_token == 0u) {
            next_device_token = 1u;
        }
        edu_device_token[instance_index] = next_device_token++;
        format_word_hex((seL4_Word)resource->bar0_paddr,
                        edu_bar_argument[instance_index]);
        format_word_hex((seL4_Word)resource->bar0_size,
                        edu_bar_len_argument[instance_index]);
        format_word_hex(edu_device_token[instance_index],
                        edu_token_argument[instance_index]);
        format_byte_hex(resource->irq_line, edu_irq_argument[instance_index]);
    }

    if (sel4utils_configure_process(&deviced, vka, root_vspace, "selinos-deviced") != 0 ||
        (has_resource &&
         sel4utils_copy_cap_to_process(&deviced, vka,
                                       edu_deviced_request_endpoint[instance_index].cptr) !=
             SELINOS_DEVICED_REQUEST_ENDPOINT)) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(deviced.thread.tcb.cptr, "selinos-deviced");
#endif
    return has_resource
               ? sel4utils_spawn_process_v(&deviced, vka, root_vspace, 5, active_argv, 1) == 0
               : sel4utils_spawn_process_v(&deviced, vka, root_vspace, 1, dormant_argv, 1) == 0;
}

static bool start_edu_dmad(vka_t *vka, vspace_t *root_vspace,
                           const sel4utils_process_t *driver_process,
                           unsigned int instance_index)
{
    sel4utils_process_t dmad;
    char *argv[] = {"selinos-dmad", edu_token_argument[instance_index], NULL};
    if (!valid_instance(instance_index) || driver_process == NULL ||
        driver_process->cspace.cptr == 0 ||
        sel4utils_configure_process(&dmad, vka, root_vspace, "selinos-dmad") != 0) {
        return false;
    }
    const seL4_CPtr free_endpoint = sel4utils_copy_cap_to_process(
        &dmad, vka, edu_dmad_free_endpoint[instance_index].cptr);
    const seL4_CPtr dma_frame = sel4utils_copy_cap_to_process(
        &dmad, vka, edu_dma_frame[instance_index].cptr);
    const seL4_CPtr driver_cnode = sel4utils_copy_cap_to_process(
        &dmad, vka, driver_process->cspace.cptr);
    if (free_endpoint != SELINOS_DMAD_FREE_ENDPOINT ||
        dma_frame != SELINOS_DMAD_DMA_FRAME_CAP ||
        driver_cnode != SELINOS_DMAD_DRIVER_CNODE_CAP) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(dmad.thread.tcb.cptr, "selinos-dmad");
#endif
    return sel4utils_spawn_process_v(&dmad, vka, root_vspace, 2, argv, 1) == 0;
}

static bool start_edu_irqd(vka_t *vka, vspace_t *root_vspace,
                           const sel4utils_process_t *driver_process,
                           unsigned int instance_index)
{
    sel4utils_process_t irqd;
    char *argv[] = {"selinos-irqd", edu_token_argument[instance_index], NULL};
    if (!valid_instance(instance_index) || driver_process == NULL ||
        driver_process->cspace.cptr == 0 ||
        sel4utils_configure_process(&irqd, vka, root_vspace, "selinos-irqd") != 0) {
        return false;
    }
    const seL4_CPtr hw_notification = sel4utils_copy_cap_to_process(
        &irqd, vka, edu_irqd_hw_notification[instance_index].cptr);
    const seL4_CPtr irq_handler = sel4utils_copy_cap_to_process(
        &irqd, vka, edu_irq_handler_cap[instance_index]);
    const seL4_CPtr completion_endpoint = sel4utils_copy_cap_to_process(
        &irqd, vka, edu_irqd_completion_endpoint[instance_index].cptr);
    const seL4_CPtr driver_notification = sel4utils_copy_cap_to_process(
        &irqd, vka, edu_driver_notification[instance_index].cptr);
    const seL4_CPtr request_endpoint = sel4utils_copy_cap_to_process(
        &irqd, vka, edu_irqd_request_endpoint[instance_index].cptr);
    const seL4_CPtr driver_cnode = sel4utils_copy_cap_to_process(
        &irqd, vka, driver_process->cspace.cptr);
    if (hw_notification != SELINOS_IRQD_HW_NOTIFICATION_CAP ||
        irq_handler != SELINOS_IRQD_HANDLER_CAP ||
        completion_endpoint != SELINOS_IRQD_COMPLETE_ENDPOINT ||
        driver_notification != SELINOS_IRQD_DRIVER_NOTIFICATION ||
        request_endpoint != SELINOS_IRQD_REQUEST_ENDPOINT ||
        driver_cnode != SELINOS_IRQD_DRIVER_CNODE_CAP) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(irqd.thread.tcb.cptr, "selinos-irqd");
#endif
    return sel4utils_spawn_process_v(&irqd, vka, root_vspace, 2, argv, 1) == 0;
}

bool selinos_start_edu_driver_domain(vka_t *vka, vspace_t *root_vspace,
                                     const struct selinos_qemu_edu_resource *resource,
                                     unsigned int instance_index)
{
    if (vka == NULL || root_vspace == NULL || resource == NULL ||
        !valid_instance(instance_index) ||
        edu_deviced_request_endpoint[instance_index].cptr == 0 ||
        resource->bar0_size != SELINOS_QEMU_EDU_BAR0_SIZE ||
        resource->irq_line == 0xffu ||
        (resource->bar0_paddr & ((1ul << seL4_PageBits) - 1ul)) != 0u ||
        !issue_edu_irq(vka, instance_index, resource->irq_line) ||
        !issue_edu_dma_lease(vka, instance_index)) {
        return false;
    }

    sel4utils_process_t process;
    sel4utils_process_config_t config = process_config_default(
        "selinos-edu-kapi-probe", seL4_CapInitThreadASIDPool);
    config = process_config_create_cnode(config, SELINOS_EDU_CSPACE_BITS);
    if (sel4utils_configure_process_custom(&process, vka, root_vspace, config) != 0) {
        return false;
    }

    reservation_t reservation = vspace_reserve_range_at(&process.vspace,
                                                         SELINOS_EDU_DRIVER_VADDR,
                                                         resource->bar0_size,
                                                         seL4_AllRights, 0);
    if (reservation.res == NULL) {
        return false;
    }
    for (size_t page = 0u; page < SELINOS_EDU_PAGE_COUNT; ++page) {
        const uintptr_t paddr = resource->bar0_paddr + page * (1ul << seL4_PageBits);
        if (vka_alloc_frame_at(vka, seL4_PageBits, paddr,
                               &edu_bar_frames[instance_index][page]) != 0) {
            return false;
        }
        edu_root_frame_caps[instance_index][page] =
            edu_bar_frames[instance_index][page].cptr;
        edu_driver_frame_caps[instance_index][page] = sel4utils_copy_cap_to_process(
            &process, vka, edu_root_frame_caps[instance_index][page]);
        if (edu_driver_frame_caps[instance_index][page] == 0) {
            return false;
        }
    }
    if (vspace_map_pages_at_vaddr(&process.vspace, edu_root_frame_caps[instance_index],
                                  NULL, SELINOS_EDU_DRIVER_VADDR,
                                  SELINOS_EDU_PAGE_COUNT, seL4_PageBits,
                                  reservation) != 0) {
        return false;
    }

    reservation = vspace_reserve_range_at(&process.vspace,
                                          (void *)SELINOS_EDU_DRIVER_DMA_VADDR,
                                          1ul << seL4_PageBits, seL4_AllRights, 0);
    if (reservation.res == NULL) {
        return false;
    }
    edu_dma_frame_cap[instance_index] = sel4utils_copy_cap_to_process(
        &process, vka, edu_dma_frame[instance_index].cptr);
    if (edu_dma_frame_cap[instance_index] != SELINOS_EDU_DRIVER_DMA_FRAME_CAP ||
        vspace_map_pages_at_vaddr(&process.vspace,
                                  &edu_dma_frame[instance_index].cptr, NULL,
                                  (void *)SELINOS_EDU_DRIVER_DMA_VADDR, 1,
                                  seL4_PageBits, reservation) != 0) {
        return false;
    }

    if (!mint_edu_control_cap(vka, instance_index,
                              &edu_driver_register_control_cap[instance_index],
                              SELINOS_IRQD_CONTROL_REGISTER_BADGE) ||
        !mint_edu_control_cap(vka, instance_index,
                              &edu_driver_unregister_control_cap[instance_index],
                              SELINOS_IRQD_CONTROL_UNREGISTER_BADGE)) {
        return false;
    }

    const seL4_CPtr driver_notification = sel4utils_copy_cap_to_process(
        &process, vka, edu_driver_notification[instance_index].cptr);
    const seL4_CPtr completion_endpoint = sel4utils_copy_cap_to_process(
        &process, vka, edu_irqd_completion_endpoint[instance_index].cptr);
    const seL4_CPtr request_endpoint = sel4utils_copy_cap_to_process(
        &process, vka, edu_irqd_request_endpoint[instance_index].cptr);
    const seL4_CPtr dmad_free_endpoint = sel4utils_copy_cap_to_process(
        &process, vka, edu_dmad_free_endpoint[instance_index].cptr);
    const seL4_CPtr register_control = sel4utils_copy_cap_to_process(
        &process, vka, edu_driver_register_control_cap[instance_index]);
    const seL4_CPtr unregister_control = sel4utils_copy_cap_to_process(
        &process, vka, edu_driver_unregister_control_cap[instance_index]);
    const seL4_CPtr deviced_request_endpoint = sel4utils_copy_cap_to_process(
        &process, vka, edu_deviced_request_endpoint[instance_index].cptr);
    if (driver_notification != SELINOS_EDU_DRIVER_NOTIFICATION_CAP ||
        completion_endpoint != SELINOS_EDU_DRIVER_COMPLETE_ENDPOINT ||
        request_endpoint != SELINOS_EDU_DRIVER_REQUEST_ENDPOINT ||
        dmad_free_endpoint != SELINOS_EDU_DRIVER_DMA_FREE_ENDPOINT ||
        register_control != SELINOS_EDU_DRIVER_REGISTER_CONTROL ||
        unregister_control != SELINOS_EDU_DRIVER_UNREGISTER_CONTROL ||
        deviced_request_endpoint != SELINOS_EDU_DRIVER_DEVICED_ENDPOINT ||
        !start_edu_dmad(vka, root_vspace, &process, instance_index) ||
        !start_edu_irqd(vka, root_vspace, &process, instance_index)) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(process.thread.tcb.cptr, "selinos-edu-kapi-probe");
#endif
    char *argv[] = {"selinos-edu-kapi-probe", edu_dma_argument[instance_index],
                    edu_irq_argument[instance_index], NULL};
    if (sel4utils_spawn_process_v(&process, vka, root_vspace, 3, argv, 1) != 0) {
        return false;
    }
    debug_puts("SeLinOS M3: independent BAR0, mediated IRQ and DMA lease granted to edu driver domain.\n");
    return true;
}
