// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#include "selinos_dmad_protocol.h"
#include "selinos_irqd_protocol.h"
#include "selinos_kapi_runtime.h"

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
#define EDU_EXPECTED_LOW_BYTE     0xedu /* QEMU edu specification marker */
#define EDU_DMA_MASK              0x0fffffffull
#define EDU_DMA_TEST_LEASE_SIZE   256u

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

    if (text == 0 || address == 0) {
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
        value = (value << 4) | digit;
    }
    if (text[digits] != '\0') {
        return 0;
    }
    *address = value;
    return 1;
}

static unsigned int edu_last_irq_status;
static struct device edu_kapi_device = {
    .init_name = "selinos-edu-mmio-driver",
};
static struct device edu_untrusted_device = {
    .init_name = "selinos-edu-untrusted",
};

static irqreturn_t edu_irq_handler(int irq, void *dev_id)
{
    (void)irq;
    (void)dev_id;

    edu_last_irq_status = *edu_register(EDU_INTERRUPT_STATUS_OFF);
    *edu_register(EDU_INTERRUPT_ACK_OFF) = edu_last_irq_status;
    return IRQ_HANDLED;
}

static int wait_mediated_irq(unsigned int expected_status)
{
    return selinos_kapi_dispatch_one_irq() == 0 &&
           (edu_last_irq_status & expected_status) == expected_status;
}

static int edu_dma_roundtrip(volatile unsigned char *dma, unsigned int dma_address)
{
    const unsigned int count = 100u;
    const unsigned int destination_offset = 128u;

    if (dma_address > 0x0ffffffful - destination_offset - count) {
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
    if (!wait_mediated_irq(0x100u) ||
        (*edu_register(EDU_DMA_COMMAND_OFF) & 0x1u) != 0u) {
        seL4_DebugPutString("SeLinOS edu driver: RAM-to-EDU DMA completion failed.\n");
        return 0;
    }

    *edu_register(EDU_DMA_SOURCE_OFF) = 0x40000u;
    *edu_register(EDU_DMA_DESTINATION_OFF) = dma_address + destination_offset;
    *edu_register(EDU_DMA_COUNT_OFF) = count;
    *edu_register(EDU_DMA_COMMAND_OFF) = 0x7u;
    if (!wait_mediated_irq(0x100u) ||
        (*edu_register(EDU_DMA_COMMAND_OFF) & 0x1u) != 0u) {
        seL4_DebugPutString("SeLinOS edu driver: EDU-to-RAM DMA completion failed.\n");
        return 0;
    }

    for (unsigned int i = 0; i < count; ++i) {
        if (dma[destination_offset + i] != (unsigned char)(i ^ 0xa5u)) {
            seL4_DebugPutString("SeLinOS edu driver: DMA data mismatch at offset 0x");
            debug_hex32(i);
            seL4_DebugPutString(".\n");
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    unsigned int dma_address = 0;
    const unsigned int identification = *edu_register(EDU_IDENTIFICATION_OFFSET);
    const unsigned int sample = 0x13579bdfu;

    unsigned int irq_line = 0;
    dma_addr_t dma_handle = 0;
    volatile unsigned char *dma_cpu_address = NULL;
    if (argc != 3 || !parse_hex_value(argv[1], 8u, &dma_address) ||
        !parse_hex_value(argv[2], 2u, &irq_line) ||
        selinos_kapi_bind_irq(irq_line, SELINOS_EDU_DRIVER_NOTIFICATION_CAP,
                              SELINOS_EDU_DRIVER_COMPLETE_ENDPOINT,
                              SELINOS_EDU_DRIVER_REQUEST_ENDPOINT,
                              SELINOS_EDU_DRIVER_REGISTER_CONTROL,
                              SELINOS_EDU_DRIVER_UNREGISTER_CONTROL) != 0 ||
        request_irq(irq_line, edu_irq_handler, 0, "selinos-edu", &edu_last_irq_status) != 0 ||
        selinos_kapi_bind_dma_lease(&edu_kapi_device, SELINOS_EDU_DRIVER_DMA_VADDR,
                                    (dma_addr_t)dma_address, EDU_DMA_TEST_LEASE_SIZE,
                                    EDU_DMA_MASK,
                                    SELINOS_EDU_DRIVER_DMA_FREE_ENDPOINT) != 0 ||
        dma_set_mask_and_coherent(&edu_kapi_device, EDU_DMA_MASK) != 0) {
        seL4_DebugPutString("SeLinOS edu driver: unable to establish KAPI IRQ/DMA lease.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    dma_cpu_address = dma_alloc_coherent(&edu_kapi_device, EDU_DMA_TEST_LEASE_SIZE,
                                         &dma_handle, GFP_KERNEL);
    if (dma_cpu_address == NULL) {
        seL4_DebugPutString("SeLinOS edu driver: KAPI dma_alloc_coherent failed.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    if (request_irq(irq_line, edu_irq_handler, 0, "duplicate", &edu_last_irq_status) == 0 ||
        dma_set_mask_and_coherent(&edu_untrusted_device, EDU_DMA_MASK) == 0 ||
        dma_set_mask_and_coherent(&edu_kapi_device, EDU_DMA_MASK + 1u) == 0 ||
        dma_alloc_coherent(&edu_kapi_device, EDU_DMA_TEST_LEASE_SIZE, &dma_handle,
                           GFP_KERNEL) != NULL) {
        seL4_DebugPutString("SeLinOS edu driver: KAPI guard check failed.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    dma_free_coherent(&edu_kapi_device, EDU_DMA_TEST_LEASE_SIZE - 1u,
                      (void *)dma_cpu_address, dma_handle);
    if (dma_alloc_coherent(&edu_kapi_device, EDU_DMA_TEST_LEASE_SIZE, &dma_handle,
                           GFP_KERNEL) != NULL) {
        seL4_DebugPutString("SeLinOS edu driver: KAPI invalid-free guard failed.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }

    seL4_DebugPutString("SeLinOS edu driver: KAPI request_irq and dma_alloc_coherent passed.\n");
    seL4_DebugPutString("SeLinOS edu driver: KAPI guard checks passed.\n");

    if ((identification & 0xffu) != EDU_EXPECTED_LOW_BYTE) {
        seL4_DebugPutString("SeLinOS edu driver: unexpected identification 0x");
        debug_hex32(identification);
        seL4_DebugPutString(".\n");
    } else {
        *edu_register(EDU_LIVENESS_OFFSET) = sample;
        const unsigned int reply = *edu_register(EDU_LIVENESS_OFFSET);
        if (reply == ~sample) {
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

    dma_free_coherent(&edu_kapi_device, EDU_DMA_TEST_LEASE_SIZE,
                      (void *)dma_cpu_address, dma_handle);
    free_irq(irq_line, &edu_last_irq_status);
    for (;;) {
        (void)seL4_Yield();
    }
}
