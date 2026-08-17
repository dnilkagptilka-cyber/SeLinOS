// SPDX-License-Identifier: MIT
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <sel4/sel4.h>

#include "selinos_deviced_protocol.h"
#include "selinos_dmad_protocol.h"
#include "selinos_irqd_protocol.h"
#include "selinos_kapi_runtime.h"

struct selinos_kapi_irq_binding {
    unsigned int irq;
    irq_handler_t handler;
    void *dev_id;
    seL4_CPtr notification_cap;
    seL4_CPtr completion_endpoint;
    seL4_CPtr request_endpoint;
    seL4_CPtr register_control;
    seL4_CPtr unregister_control;
    u64 device_token;
};

static struct selinos_kapi_irq_binding selinos_irq_binding;

struct selinos_kapi_dma_lease {
    struct device *device;
    void *cpu_address;
    dma_addr_t dma_address;
    size_t size;
    u64 supported_mask;
    u64 selected_mask;
    size_t allocated_size;
    seL4_CPtr free_endpoint;
    int configured;
    int allocated;
    int revoked;
};

struct selinos_kapi_pci_binding {
    seL4_CPtr request_endpoint;
    struct pci_driver *driver;
    const struct pci_device_id *matched_id;
    struct pci_dev shadow_pdev;
    int probing;
    int active;
};

#define SELINOS_PCI_FLAG_ENABLED 0x1u
#define SELINOS_PCI_FLAG_MASTER  0x2u

static struct selinos_kapi_dma_lease selinos_dma_lease;
static struct selinos_kapi_pci_binding selinos_pci_binding;

const char *dev_name(const struct device *dev)
{
    return (dev != NULL && dev->init_name != NULL) ? dev->init_name : "(unnamed)";
}

int dev_err(const struct device *dev, const char *message)
{
    (void)dev;
    (void)message;
    return 0;
}

int dev_info(const struct device *dev, const char *message)
{
    (void)dev;
    (void)message;
    return 0;
}

static int selinos_pci_id_is_sentinel(const struct pci_device_id *id)
{
    return id->vendor == 0u && id->device == 0u && id->subvendor == 0u &&
           id->subdevice == 0u && id->class == 0u && id->class_mask == 0u &&
           id->driver_data == 0ul;
}

static int selinos_pci_id_is_supported(const struct pci_device_id *id)
{
    return (id->subvendor == 0u || id->subvendor == PCI_ANY_ID) &&
           (id->subdevice == 0u || id->subdevice == PCI_ANY_ID) &&
           id->class == 0u && id->class_mask == 0u;
}

int selinos_kapi_bind_deviced(unsigned long request_endpoint)
{
    if (request_endpoint == 0ul || selinos_pci_binding.request_endpoint != 0) {
        return -EINVAL;
    }
    selinos_pci_binding.request_endpoint = (seL4_CPtr)request_endpoint;
    return 0;
}

int pci_register_driver(struct pci_driver *driver)
{
    if (driver == NULL || driver->id_table == NULL || driver->probe == NULL ||
        selinos_pci_binding.request_endpoint == 0 || selinos_pci_binding.active) {
        return -EINVAL;
    }

    for (unsigned int index = 0u; index < 64u; ++index) {
        const struct pci_device_id *id = &driver->id_table[index];
        if (selinos_pci_id_is_sentinel(id)) {
            break;
        }
        if (!selinos_pci_id_is_supported(id)) {
            continue;
        }

        seL4_SetMR(0, SELINOS_DEVICED_REGISTER_MAGIC);
        seL4_SetMR(1, id->vendor);
        seL4_SetMR(2, id->device);
        seL4_MessageInfo_t reply = seL4_Call(
            selinos_pci_binding.request_endpoint,
            seL4_MessageInfo_new(0, 0, 0, SELINOS_DEVICED_REGISTER_REQUEST_WORDS));
        const seL4_Word reply_length = seL4_MessageInfo_get_length(reply);

        if (reply_length == 1u && seL4_GetMR(0) == SELINOS_DEVICED_REGISTER_NO_MATCH) {
            continue;
        }
        if (reply_length != SELINOS_DEVICED_REGISTER_REPLY_WORDS ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_MAGIC_INDEX) !=
                SELINOS_DEVICED_REGISTER_ACCEPTED ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_TOKEN_INDEX) == 0u ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_VENDOR_INDEX) !=
                SELINOS_DEVICED_QEMU_EDU_VENDOR ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_DEVICE_INDEX) !=
                SELINOS_DEVICED_QEMU_EDU_DEVICE ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_IRQ_INDEX) == 0u ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_IRQ_INDEX) > 0xffu ||
            seL4_GetMR(SELINOS_DEVICED_REPLY_BAR0_LEN_INDEX) == 0u) {
            return -EINVAL;
        }

        selinos_pci_binding.shadow_pdev = (struct pci_dev){0};
        selinos_pci_binding.shadow_pdev.dev.init_name = driver->name;
        selinos_pci_binding.shadow_pdev.dev.selinos_device_token =
            (u64)seL4_GetMR(SELINOS_DEVICED_REPLY_TOKEN_INDEX);
        selinos_pci_binding.shadow_pdev.vendor = SELINOS_DEVICED_QEMU_EDU_VENDOR;
        selinos_pci_binding.shadow_pdev.device = SELINOS_DEVICED_QEMU_EDU_DEVICE;
        selinos_pci_binding.shadow_pdev.irq =
            (u8)seL4_GetMR(SELINOS_DEVICED_REPLY_IRQ_INDEX);
        selinos_pci_binding.shadow_pdev.resource_start[0] =
            (u64)seL4_GetMR(SELINOS_DEVICED_REPLY_BAR0_START_INDEX);
        selinos_pci_binding.shadow_pdev.resource_len[0] =
            (u64)seL4_GetMR(SELINOS_DEVICED_REPLY_BAR0_LEN_INDEX);

        if (selinos_dma_lease.configured && selinos_dma_lease.device == NULL) {
            selinos_dma_lease.device = &selinos_pci_binding.shadow_pdev.dev;
        }
        if (selinos_irq_binding.irq == selinos_pci_binding.shadow_pdev.irq) {
            selinos_irq_binding.device_token =
                selinos_pci_binding.shadow_pdev.dev.selinos_device_token;
        }
        selinos_pci_binding.driver = driver;
        selinos_pci_binding.matched_id = id;
        selinos_pci_binding.probing = 1;
        const int probe_status = driver->probe(&selinos_pci_binding.shadow_pdev, id);
        selinos_pci_binding.probing = 0;
        if (probe_status != 0) {
            if (!selinos_dma_lease.allocated) {
                selinos_dma_lease.device = NULL;
            }
            selinos_irq_binding.device_token = 0u;
            selinos_pci_binding.driver = NULL;
            selinos_pci_binding.matched_id = NULL;
            selinos_pci_binding.shadow_pdev = (struct pci_dev){0};
            return probe_status;
        }

        selinos_pci_binding.active = 1;
        return 0;
    }
    return -ENODEV;
}

void pci_unregister_driver(struct pci_driver *driver)
{
    if (driver == NULL || driver != selinos_pci_binding.driver ||
        !selinos_pci_binding.active) {
        return;
    }
    if (driver->remove != NULL) {
        driver->remove(&selinos_pci_binding.shadow_pdev);
    }
    if (!selinos_dma_lease.allocated) {
        selinos_dma_lease.device = NULL;
        selinos_dma_lease.selected_mask = 0u;
    }
    selinos_irq_binding.device_token = 0u;
    selinos_pci_binding.driver = NULL;
    selinos_pci_binding.matched_id = NULL;
    selinos_pci_binding.active = 0;
    selinos_pci_binding.shadow_pdev = (struct pci_dev){0};
}

int pci_enable_device(struct pci_dev *pdev)
{
    if (pdev == NULL || pdev != &selinos_pci_binding.shadow_pdev ||
        (!selinos_pci_binding.active && !selinos_pci_binding.probing) ||
        pdev->dev.selinos_device_token == 0u) {
        return -EINVAL;
    }
    pdev->dev.selinos_flags |= SELINOS_PCI_FLAG_ENABLED;
    return 0;
}

void pci_disable_device(struct pci_dev *pdev)
{
    if (pdev != &selinos_pci_binding.shadow_pdev) {
        return;
    }
    pdev->dev.selinos_flags &= ~(SELINOS_PCI_FLAG_ENABLED | SELINOS_PCI_FLAG_MASTER);
}

int pci_set_master(struct pci_dev *pdev)
{
    if (pdev == NULL || pdev != &selinos_pci_binding.shadow_pdev ||
        (pdev->dev.selinos_flags & SELINOS_PCI_FLAG_ENABLED) == 0u) {
        return -EINVAL;
    }
    pdev->dev.selinos_flags |= SELINOS_PCI_FLAG_MASTER;
    return 0;
}

unsigned long pci_resource_start(const struct pci_dev *pdev, unsigned int bar)
{
    return (pdev != NULL && bar < 6u) ? (unsigned long)pdev->resource_start[bar] : 0ul;
}

unsigned long pci_resource_len(const struct pci_dev *pdev, unsigned int bar)
{
    return (pdev != NULL && bar < 6u) ? (unsigned long)pdev->resource_len[bar] : 0ul;
}

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
                const char *name, void *dev_id)
{
    (void)flags;
    (void)name;

    if (handler == NULL || selinos_irq_binding.notification_cap == 0 ||
        selinos_irq_binding.completion_endpoint == 0 ||
                selinos_irq_binding.request_endpoint == 0 ||
        selinos_irq_binding.handler != NULL ||
        selinos_irq_binding.irq != irq || selinos_irq_binding.device_token == 0u) {
        return -EINVAL;
    }

    seL4_Signal(selinos_irq_binding.register_control);
    seL4_SetMR(0, SELINOS_IRQD_REGISTER_MAGIC);
    seL4_SetMR(1, irq);
    seL4_SetMR(2, selinos_irq_binding.device_token);
    seL4_MessageInfo_t reply = seL4_Call(selinos_irq_binding.request_endpoint,
                                         seL4_MessageInfo_new(0, 0, 0,
                                                              SELINOS_IRQD_CONTROL_REQUEST_WORDS));
    if (seL4_MessageInfo_get_length(reply) != 1u ||
        seL4_GetMR(0) != SELINOS_IRQD_REGISTER_ACCEPTED) {
        return -EINVAL;
    }

    selinos_irq_binding.handler = handler;
    selinos_irq_binding.dev_id = dev_id;
    return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
    if (selinos_irq_binding.handler == NULL || selinos_irq_binding.irq != irq ||
        selinos_irq_binding.dev_id != dev_id) {
        return;
    }

    if (selinos_irq_binding.device_token == 0u) {
        return;
    }
    seL4_Signal(selinos_irq_binding.unregister_control);
    seL4_SetMR(0, SELINOS_IRQD_UNREGISTER_MAGIC);
    seL4_SetMR(1, irq);
    seL4_SetMR(2, selinos_irq_binding.device_token);
    seL4_MessageInfo_t reply = seL4_Call(selinos_irq_binding.request_endpoint,
                                         seL4_MessageInfo_new(0, 0, 0,
                                                              SELINOS_IRQD_CONTROL_REQUEST_WORDS));
    if (seL4_MessageInfo_get_length(reply) != 1u ||
        seL4_GetMR(0) != SELINOS_IRQD_UNREGISTER_ACCEPTED) {
        return;
    }

    selinos_irq_binding.handler = NULL;
    selinos_irq_binding.dev_id = NULL;
}

int selinos_kapi_bind_irq(unsigned int irq, unsigned long notification_cap,
                          unsigned long completion_endpoint, unsigned long request_endpoint,
                          unsigned long register_control, unsigned long unregister_control)
{
    if (irq == 0u || notification_cap == 0ul || completion_endpoint == 0ul ||
        request_endpoint == 0ul || register_control == 0ul || unregister_control == 0ul ||
        selinos_irq_binding.notification_cap != 0 ||
        selinos_irq_binding.completion_endpoint != 0 ||
        selinos_irq_binding.request_endpoint != 0 ||
        selinos_irq_binding.register_control != 0 ||
        selinos_irq_binding.unregister_control != 0) {
        return -EINVAL;
    }

    selinos_irq_binding.irq = irq;
    selinos_irq_binding.notification_cap = (seL4_CPtr)notification_cap;
    selinos_irq_binding.completion_endpoint = (seL4_CPtr)completion_endpoint;
    selinos_irq_binding.request_endpoint = (seL4_CPtr)request_endpoint;
    selinos_irq_binding.register_control = (seL4_CPtr)register_control;
    selinos_irq_binding.unregister_control = (seL4_CPtr)unregister_control;
    return 0;
}

int selinos_kapi_dispatch_one_irq(void)
{
    seL4_Word badge = 0;

    if (selinos_irq_binding.handler == NULL || selinos_irq_binding.notification_cap == 0 ||
        selinos_irq_binding.completion_endpoint == 0) {
        return -EINVAL;
    }

    (void)seL4_Wait(selinos_irq_binding.notification_cap, &badge);
    (void)selinos_irq_binding.handler((int)selinos_irq_binding.irq,
                                      selinos_irq_binding.dev_id);
    seL4_SetMR(0, SELINOS_IRQD_COMPLETE_MAGIC);
    seL4_Send(selinos_irq_binding.completion_endpoint,
              seL4_MessageInfo_new(0, 0, 0, 1));
    return 0;
}

int selinos_kapi_bind_dma_lease(struct device *dev, unsigned long cpu_address,
                                dma_addr_t dma_address, size_t size, u64 dma_mask,
                                unsigned long free_endpoint)
{
    if (cpu_address == 0ul || dma_address == 0ul || size == 0u ||
        dma_mask == 0u || free_endpoint == 0ul || selinos_dma_lease.configured ||
        dma_address > dma_mask || size - 1u > dma_mask - dma_address) {
        return -EINVAL;
    }

    selinos_dma_lease.device = dev;
    selinos_dma_lease.cpu_address = (void *)cpu_address;
    selinos_dma_lease.dma_address = dma_address;
    selinos_dma_lease.size = size;
    selinos_dma_lease.supported_mask = dma_mask;
    selinos_dma_lease.free_endpoint = (seL4_CPtr)free_endpoint;
    selinos_dma_lease.configured = 1;
    return 0;
}

void disable_irq(unsigned int irq)
{
    (void)irq;
}

void enable_irq(unsigned int irq)
{
    (void)irq;
}

int dma_set_mask_and_coherent(struct device *dev, u64 mask)
{
    if (dev == NULL || dev != selinos_dma_lease.device || selinos_dma_lease.revoked ||
        mask == 0u || mask > selinos_dma_lease.supported_mask ||
        selinos_dma_lease.dma_address > mask ||
        selinos_dma_lease.size - 1u > mask - selinos_dma_lease.dma_address) {
        return -EINVAL;
    }

    selinos_dma_lease.selected_mask = mask;
    return 0;
}

void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle,
                         gfp_t flags)
{
    (void)flags;

    if (dev == NULL || dma_handle == NULL || dev != selinos_dma_lease.device ||
        selinos_dma_lease.revoked || selinos_dma_lease.selected_mask == 0u ||
        selinos_dma_lease.allocated ||
        size == 0u || size > selinos_dma_lease.size) {
        return NULL;
    }

    selinos_dma_lease.allocated_size = size;
    selinos_dma_lease.allocated = 1;
    *dma_handle = selinos_dma_lease.dma_address;
    return selinos_dma_lease.cpu_address;
}

void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr,
                       dma_addr_t dma_handle)
{
    if (dev != selinos_dma_lease.device || !selinos_dma_lease.allocated ||
        size != selinos_dma_lease.allocated_size ||
        cpu_addr != selinos_dma_lease.cpu_address ||
        dma_handle != selinos_dma_lease.dma_address) {
        return;
    }

    if (dev->selinos_device_token == 0u) {
        return;
    }
    seL4_SetMR(0, SELINOS_DMAD_FREE_MAGIC);
    seL4_SetMR(1, dev->selinos_device_token);
    seL4_SetMR(2, dma_handle);
    seL4_SetMR(3, size);
    seL4_MessageInfo_t reply = seL4_Call(selinos_dma_lease.free_endpoint,
                                         seL4_MessageInfo_new(0, 0, 0, 4));
    if (seL4_MessageInfo_get_length(reply) != 1u ||
        seL4_GetMR(0) != SELINOS_DMAD_FREE_ACCEPTED) {
        return;
    }

    selinos_dma_lease.allocated_size = 0u;
    selinos_dma_lease.allocated = 0;
    selinos_dma_lease.revoked = 1;
}

void complete(struct completion *completion)
{
    if (completion != NULL && completion->done != ~0u) {
        ++completion->done;
    }
}

unsigned long wait_for_completion_timeout(struct completion *completion,
                                          unsigned long timeout)
{
    (void)timeout;
    if (completion == NULL || completion->done == 0u) {
        return 0ul;
    }
    --completion->done;
    return 1ul;
}

bool schedule_work(struct work_struct *work)
{
    if (work == NULL || work->func == NULL || work->pending != 0u) {
        return false;
    }
    work->pending = 1u;
    work->func(work);
    work->pending = 0u;
    return true;
}

bool cancel_work_sync(struct work_struct *work)
{
    if (work == NULL || work->pending == 0u) {
        return false;
    }
    work->pending = 0u;
    return true;
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
    int was_pending;

    if (timer == NULL || timer->function == NULL) {
        return 0;
    }
    was_pending = timer->pending != 0u;
    timer->expires = expires;
    timer->pending = 1u;
    return was_pending;
}

int del_timer_sync(struct timer_list *timer)
{
    int was_pending;

    if (timer == NULL) {
        return 0;
    }
    was_pending = timer->pending != 0u;
    timer->pending = 0u;
    return was_pending;
}

unsigned long spin_lock_irqsave(spinlock_t *lock)
{
    if (lock != NULL) {
        lock->locked = 1u;
    }
    return 0ul;
}

void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
    (void)flags;
    if (lock != NULL) {
        lock->locked = 0u;
    }
}

void *kmalloc(size_t size, gfp_t flags)
{
    (void)size;
    (void)flags;
    return NULL;
}

void *kzalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags);
}

void kfree(const void *ptr)
{
    (void)ptr;
}

/* M2 no-authority gate: the current profile has no audited device-frame
 * mapping contract, so canonical ioremap fails closed before any seL4
 * operation. This preserves the existing device-token/IRQ/DMA paths without
 * manufacturing a device virtual address. */
void *ioremap(unsigned long physical_address, size_t size)
{
    (void)physical_address;
    (void)size;
    return NULL;
}

void iounmap(void *address)
{
    (void)address;
}
