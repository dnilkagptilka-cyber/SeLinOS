// SPDX-License-Identifier: MIT
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <sel4/sel4.h>

#include "selinos_deviced_protocol.h"
#include "selinos_dmad_protocol.h"
#include "selinos_irqd_protocol.h"
#include "selinos_kapi_runtime.h"

#define QEMU_EDU_VENDOR_ID 0x1234u
#define QEMU_EDU_DEVICE_ID 0x11e8u
#define QEMU_EDU_DMA_MASK  0x0fffffffull
#define EDU_DMA_TEST_LEASE_SIZE 256u

#define EDU_MMIO_BASE ((volatile unsigned int *)0x600000000000ull)
#define EDU_IDENTIFICATION_OFFSET 0x00u
#define EDU_LIVENESS_OFFSET       0x04u
#define EDU_INTERRUPT_STATUS_OFF  0x24u
#define EDU_INTERRUPT_RAISE_OFF   0x60u
#define EDU_INTERRUPT_ACK_OFF     0x64u
#define EDU_DMA_SOURCE_OFF        0x80u
#define EDU_DMA_DESTINATION_OFF   0x88u
#define EDU_DMA_COUNT_OFF         0x90u
#define EDU_DMA_COMMAND_OFF       0x98u
#define EDU_EXPECTED_LOW_BYTE     0xedu

static unsigned int edu_last_irq_status;
static unsigned int edu_phase6_work_runs;
static unsigned int edu_phase6_timer_runs;
static struct completion edu_phase6_completion;
static struct work_struct edu_phase6_work;
static struct timer_list edu_phase6_timer;
static struct pci_dev *edu_bound_pdev;
static struct device edu_untrusted_device = {
    .init_name = "selinos-edu-untrusted",
};

static volatile unsigned int *edu_register(unsigned int byte_offset)
{
    return (volatile unsigned int *)((unsigned long)EDU_MMIO_BASE + byte_offset);
}

static void debug_hex32(unsigned int value)
{
    static const char alphabet[] = "0123456789abcdef";
    for (int shift = 28; shift >= 0; shift -= 4) {
        seL4_DebugPutChar(alphabet[(value >> (unsigned int)shift) & 0xfu]);
    }
}

static int parse_hex_value(const char *text, unsigned int digits, unsigned int *address)
{
    unsigned int value = 0;

    if (text == NULL || address == NULL) {
        return 0;
    }
    for (unsigned int i = 0; i < digits; ++i) {
        const char c = text[i];
        unsigned int digit;
        if (c >= '0' && c <= '9') {
            digit = (unsigned int)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (unsigned int)(c - 'a') + 10u;
        } else {
            return 0;
        }
        value = (value << 4u) | digit;
    }
    if (text[digits] != '\0') {
        return 0;
    }
    *address = value;
    return 1;
}

static irqreturn_t edu_irq_handler(int irq, void *opaque)
{
    (void)irq;
    (void)opaque;
    edu_last_irq_status = *edu_register(EDU_INTERRUPT_STATUS_OFF);
    *edu_register(EDU_INTERRUPT_ACK_OFF) = edu_last_irq_status;
    return IRQ_HANDLED;
}

static int wait_mediated_irq(unsigned int expected_status)
{
    return selinos_kapi_dispatch_one_irq() == 0 &&
           (edu_last_irq_status & expected_status) == expected_status;
}

static void edu_phase6_work_callback(struct work_struct *work)
{
    (void)work;
    ++edu_phase6_work_runs;
    complete(&edu_phase6_completion);
}

static void edu_phase6_timer_callback(struct timer_list *timer)
{
    (void)timer;
    ++edu_phase6_timer_runs;
}

static int edu_phase6_sync_primitives_pass(void)
{
    spinlock_t lock;
    unsigned long irq_flags;

    edu_phase6_work_runs = 0u;
    edu_phase6_timer_runs = 0u;
    init_completion(&edu_phase6_completion);
    INIT_WORK(&edu_phase6_work, edu_phase6_work_callback);
    timer_setup(&edu_phase6_timer, edu_phase6_timer_callback, 0u);
    spin_lock_init(&lock);

    irq_flags = spin_lock_irqsave(&lock);
    if (lock.locked != 1u) {
        return 0;
    }
    spin_unlock_irqrestore(&lock, irq_flags);
    if (lock.locked != 0u || wait_for_completion_timeout(&edu_phase6_completion, 1ul) != 0ul ||
        !schedule_work(&edu_phase6_work) || edu_phase6_work_runs != 1u ||
        wait_for_completion_timeout(&edu_phase6_completion, 1ul) != 1ul ||
        wait_for_completion_timeout(&edu_phase6_completion, 1ul) != 0ul ||
        cancel_work_sync(&edu_phase6_work)) {
        return 0;
    }
    if (mod_timer(&edu_phase6_timer, 5ul) != 0 || edu_phase6_timer.pending != 1u ||
        edu_phase6_timer.expires != 5ul || mod_timer(&edu_phase6_timer, 9ul) != 1 ||
        edu_phase6_timer.expires != 9ul || edu_phase6_timer_runs != 0u ||
        del_timer_sync(&edu_phase6_timer) != 1 || edu_phase6_timer.pending != 0u ||
        del_timer_sync(&edu_phase6_timer) != 0) {
        return 0;
    }
    return 1;
}

static int forged_irq_token_is_rejected(const struct pci_dev *pdev)
{
    seL4_Signal(SELINOS_EDU_DRIVER_REGISTER_CONTROL);
    seL4_SetMR(0, SELINOS_IRQD_REGISTER_MAGIC);
    seL4_SetMR(1, pdev->irq);
    seL4_SetMR(2, pdev->dev.selinos_device_token + 1u);
    seL4_MessageInfo_t reply = seL4_Call(SELINOS_EDU_DRIVER_REQUEST_ENDPOINT,
                                         seL4_MessageInfo_new(
                                             0, 0, 0,
                                             SELINOS_IRQD_CONTROL_REQUEST_WORDS));
    return seL4_MessageInfo_get_length(reply) == 1u && seL4_GetMR(0) == 0u;
}

static int forged_dma_token_is_rejected(const struct pci_dev *pdev,
                                        dma_addr_t dma_handle, size_t size)
{
    seL4_SetMR(0, SELINOS_DMAD_FREE_MAGIC);
    seL4_SetMR(1, pdev->dev.selinos_device_token + 1u);
    seL4_SetMR(2, dma_handle);
    seL4_SetMR(3, size);
    seL4_MessageInfo_t reply = seL4_Call(SELINOS_EDU_DRIVER_DMA_FREE_ENDPOINT,
                                         seL4_MessageInfo_new(0, 0, 0, 4));
    return seL4_MessageInfo_get_length(reply) == 1u && seL4_GetMR(0) == 0u;
}

static int edu_dma_roundtrip(volatile unsigned char *dma, unsigned int dma_address)
{
    const unsigned int count = 100u;
    const unsigned int destination_offset = 128u;

    if (dma_address > QEMU_EDU_DMA_MASK - destination_offset - count) {
        return 0;
    }
    for (unsigned int i = 0; i < count; ++i) {
        dma[i] = (unsigned char)(i ^ 0xa5u);
        dma[destination_offset + i] = 0u;
    }

    *edu_register(EDU_DMA_SOURCE_OFF) = dma_address;
    *edu_register(EDU_DMA_DESTINATION_OFF) = 0x40000u;
    *edu_register(EDU_DMA_COUNT_OFF) = count;
    *edu_register(EDU_DMA_COMMAND_OFF) = 0x5u;
    if (!wait_mediated_irq(0x100u) || (*edu_register(EDU_DMA_COMMAND_OFF) & 0x1u) != 0u) {
        return 0;
    }

    *edu_register(EDU_DMA_SOURCE_OFF) = 0x40000u;
    *edu_register(EDU_DMA_DESTINATION_OFF) = dma_address + destination_offset;
    *edu_register(EDU_DMA_COUNT_OFF) = count;
    *edu_register(EDU_DMA_COMMAND_OFF) = 0x7u;
    if (!wait_mediated_irq(0x100u) || (*edu_register(EDU_DMA_COMMAND_OFF) & 0x1u) != 0u) {
        return 0;
    }

    for (unsigned int i = 0; i < count; ++i) {
        if (dma[destination_offset + i] != (unsigned char)(i ^ 0xa5u)) {
            return 0;
        }
    }
    return 1;
}

static int edu_kapi_m2_ioremap_fails_closed(unsigned long physical_address,
                                               unsigned long size)
{
    if (physical_address == 0ul || size == 0ul || ioremap(physical_address, size) != NULL) {
        return 0;
    }
    iounmap(NULL);
    return 1;
}

static int edu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    const unsigned int sample = 0x13579bdfu;
    const unsigned int identification = *edu_register(EDU_IDENTIFICATION_OFFSET);
    dma_addr_t dma_handle = 0;
    volatile unsigned char *dma_cpu_address;

    if (pdev == NULL || id == NULL || pdev->vendor != QEMU_EDU_VENDOR_ID ||
        pdev->device != QEMU_EDU_DEVICE_ID || pdev->irq == 0u ||
        pci_resource_start(pdev, 0u) == 0ul ||
        pci_resource_len(pdev, 0u) != 0x100000ul) {
        return -1;
    }
    if (!edu_kapi_m2_ioremap_fails_closed(pci_resource_start(pdev, 0u),
                                          pci_resource_len(pdev, 0u))) {
        seL4_DebugPutString("SeLinOS edu driver: KAPI M2 no-authority ioremap guard failed.\n");
        return -1;
    }
    seL4_DebugPutString("SeLinOS edu driver: KAPI M2 ioremap fails closed; no MMIO authority.\n");
    edu_bound_pdev = pdev;
    if (pci_enable_device(pdev) != 0 || pci_set_master(pdev) != 0 ||
        dma_set_mask_and_coherent(&pdev->dev, QEMU_EDU_DMA_MASK) != 0 ||
        !forged_irq_token_is_rejected(pdev) ||
        request_irq(pdev->irq, edu_irq_handler, 0, "selinos-edu", pdev) != 0) {
        pci_disable_device(pdev);
        return -1;
    }

    dma_cpu_address = dma_alloc_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE,
                                         &dma_handle, GFP_KERNEL);
    if (dma_cpu_address == NULL) {
        free_irq(pdev->irq, pdev);
        pci_disable_device(pdev);
        return -1;
    }
    if (!forged_dma_token_is_rejected(pdev, dma_handle, EDU_DMA_TEST_LEASE_SIZE) ||
        request_irq(pdev->irq, edu_irq_handler, 0, "duplicate", pdev) == 0 ||
        dma_set_mask_and_coherent(&edu_untrusted_device, QEMU_EDU_DMA_MASK) == 0 ||
        dma_set_mask_and_coherent(&pdev->dev, QEMU_EDU_DMA_MASK + 1u) == 0 ||
        dma_alloc_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE, &dma_handle,
                           GFP_KERNEL) != NULL) {
        seL4_DebugPutString("SeLinOS edu driver: KAPI guard check failed.\n");
        dma_free_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE, (void *)dma_cpu_address,
                          dma_handle);
        free_irq(pdev->irq, pdev);
        pci_disable_device(pdev);
        return -1;
    }

    if (!edu_phase6_sync_primitives_pass()) {
        seL4_DebugPutString("SeLinOS edu driver: Phase 6 KAPI synchronization shim checks failed.\n");
        dma_free_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE, (void *)dma_cpu_address,
                          dma_handle);
        free_irq(pdev->irq, pdev);
        pci_disable_device(pdev);
        return -1;
    }

    seL4_DebugPutString("SeLinOS edu driver: KAPI request_irq and dma_alloc_coherent passed.\n");
    seL4_DebugPutString("SeLinOS edu driver: KAPI guard checks passed.\n");
    seL4_DebugPutString("SeLinOS edu driver: Phase 6 completion/work/timer/spinlock shim checks passed.\n");
    seL4_DebugPutString("SeLinOS edu driver: device-token guard checks passed.\n");
    if ((identification & 0xffu) != EDU_EXPECTED_LOW_BYTE) {
        seL4_DebugPutString("SeLinOS edu driver: unexpected identification 0x");
        debug_hex32(identification);
        seL4_DebugPutString(".\n");
    } else {
        *edu_register(EDU_LIVENESS_OFFSET) = sample;
        if (*edu_register(EDU_LIVENESS_OFFSET) == ~sample) {
            seL4_DebugPutString("SeLinOS edu driver: BAR0 MMIO liveness passed.\n");
            if (edu_dma_roundtrip(dma_cpu_address, (unsigned int)dma_handle)) {
                seL4_DebugPutString("SeLinOS edu driver: mediated 100-byte coherent DMA roundtrip passed.\n");
            } else {
                seL4_DebugPutString("SeLinOS edu driver: mediated coherent DMA roundtrip failed.\n");
            }
            *edu_register(EDU_INTERRUPT_RAISE_OFF) = 0x1u;
            if (wait_mediated_irq(0x1u)) {
                seL4_DebugPutString("SeLinOS edu driver: mediated IRQ raise/wait/device-ack passed.\n");
            } else {
                seL4_DebugPutString("SeLinOS edu driver: mediated IRQ handling failed.\n");
            }
        } else {
            seL4_DebugPutString("SeLinOS edu driver: BAR0 MMIO liveness failed.\n");
        }
    }

    dma_free_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE, (void *)dma_cpu_address, dma_handle);
    if (dma_alloc_coherent(&pdev->dev, EDU_DMA_TEST_LEASE_SIZE, &dma_handle,
                           GFP_KERNEL) != NULL ||
        dma_set_mask_and_coherent(&pdev->dev, QEMU_EDU_DMA_MASK) == 0) {
        seL4_DebugPutString("SeLinOS edu driver: DMA revocation guard failed.\n");
        return -1;
    }
    seL4_DebugPutString("SeLinOS edu driver: DMA revocation guard passed.\n");
    seL4_DebugPutString("SeLinOS KAPI 6.18: deviced matched QEMU edu; local probe completed.\n");
    return 0;
}

static void edu_remove(struct pci_dev *pdev)
{
    if (pdev != edu_bound_pdev) {
        return;
    }
    free_irq(pdev->irq, pdev);
    pci_disable_device(pdev);
    edu_bound_pdev = NULL;
    seL4_DebugPutString("SeLinOS KAPI 6.18: deviced-driven QEMU edu remove completed.\n");
}

static const struct pci_device_id edu_ids[] = {
    { PCI_DEVICE(QEMU_EDU_VENDOR_ID, QEMU_EDU_DEVICE_ID) },
    { 0 },
};

static struct pci_driver edu_driver = {
    .name = "selinos-edu-kapi-probe",
    .id_table = edu_ids,
    .probe = edu_probe,
    .remove = edu_remove,
};

int main(int argc, char *argv[])
{
    unsigned int dma_address;
    unsigned int irq_line;

    if (argc == 1) {
        seL4_DebugPutString("SeLinOS KAPI 6.18: QEMU edu absent; PCI probe domain dormant.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    if (argc != 3 || !parse_hex_value(argv[1], 8u, &dma_address) ||
        !parse_hex_value(argv[2], 2u, &irq_line) ||
        selinos_kapi_bind_deviced(SELINOS_EDU_DRIVER_DEVICED_ENDPOINT) != 0 ||
        selinos_kapi_bind_irq(irq_line, SELINOS_EDU_DRIVER_NOTIFICATION_CAP,
                              SELINOS_EDU_DRIVER_COMPLETE_ENDPOINT,
                              SELINOS_EDU_DRIVER_REQUEST_ENDPOINT,
                              SELINOS_EDU_DRIVER_REGISTER_CONTROL,
                              SELINOS_EDU_DRIVER_UNREGISTER_CONTROL) != 0 ||
        selinos_kapi_bind_dma_lease(NULL, SELINOS_EDU_DRIVER_DMA_VADDR,
                                    (dma_addr_t)dma_address, EDU_DMA_TEST_LEASE_SIZE,
                                    QEMU_EDU_DMA_MASK,
                                    SELINOS_EDU_DRIVER_DMA_FREE_ENDPOINT) != 0) {
        seL4_DebugPutString("SeLinOS KAPI 6.18: root-issued driver capability bind failed.\n");
        return 1;
    }

    if (pci_register_driver(&edu_driver) != 0) {
        seL4_DebugPutString("SeLinOS KAPI 6.18: deviced registration/probe failed.\n");
        return 1;
    }
    pci_unregister_driver(&edu_driver);
    for (;;) {
        (void)seL4_Yield();
    }
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SeLinOS Linux 6.18 KAPI QEMU edu deviced compatibility probe");
MODULE_DEVICE_TABLE(pci, edu_ids);
