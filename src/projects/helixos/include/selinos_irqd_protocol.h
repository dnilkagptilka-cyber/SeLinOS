// SPDX-License-Identifier: MIT
#ifndef SELINOS_IRQD_PROTOCOL_H
#define SELINOS_IRQD_PROTOCOL_H

/*
 * Capability layout is a narrow ABI shared by the self-authored QEMU edu
 * driver and its single-device irqd instance. `8` is sel4utils'
 * SEL4UTILS_FIRST_FREE value, deliberately copied here so the driver remains
 * linked only to libsel4/sel4runtime. No domain receives IRQControl. The
 * driver receives no IRQHandler capability: only irqd can acknowledge the
 * kernel IRQ after the driver confirms that the device interrupt was cleared.
 */
#define SELINOS_PROCESS_FIRST_FREE 8u
#define SELINOS_IRQD_HW_NOTIFICATION_CAP (SELINOS_PROCESS_FIRST_FREE)
#define SELINOS_IRQD_HANDLER_CAP         (SELINOS_PROCESS_FIRST_FREE + 1u)
#define SELINOS_IRQD_COMPLETE_ENDPOINT   (SELINOS_PROCESS_FIRST_FREE + 2u)
#define SELINOS_IRQD_DRIVER_NOTIFICATION (SELINOS_PROCESS_FIRST_FREE + 3u)
#define SELINOS_IRQD_REQUEST_ENDPOINT    (SELINOS_PROCESS_FIRST_FREE + 4u)
#define SELINOS_IRQD_DRIVER_CNODE_CAP    (SELINOS_PROCESS_FIRST_FREE + 5u)
#define SELINOS_IRQD_DRIVER_CSPACE_BITS  10u

#define SELINOS_IRQD_CONTROL_REGISTER_BADGE   0x1u
#define SELINOS_IRQD_CONTROL_UNREGISTER_BADGE 0x2u

/* The EDU BAR consumes slots 8..263. The low-memory RAM lease is a single
 * explicit frame capability; it is not an allocator capability. */
#define SELINOS_EDU_DRIVER_DMA_VADDR          0x600000100000ull
#define SELINOS_EDU_DRIVER_DMA_FRAME_CAP      (SELINOS_PROCESS_FIRST_FREE + 256u)
#define SELINOS_EDU_DRIVER_NOTIFICATION_CAP   (SELINOS_PROCESS_FIRST_FREE + 257u)
#define SELINOS_EDU_DRIVER_COMPLETE_ENDPOINT  (SELINOS_PROCESS_FIRST_FREE + 258u)
#define SELINOS_EDU_DRIVER_REQUEST_ENDPOINT   (SELINOS_PROCESS_FIRST_FREE + 259u)
#define SELINOS_EDU_DRIVER_REGISTER_CONTROL   (SELINOS_PROCESS_FIRST_FREE + 261u)
#define SELINOS_EDU_DRIVER_UNREGISTER_CONTROL (SELINOS_PROCESS_FIRST_FREE + 262u)

/* Register/unregister request: magic, IRQ line, opaque device-generation token. */
#define SELINOS_IRQD_CONTROL_REQUEST_WORDS 3u

#define SELINOS_IRQD_COMPLETE_MAGIC 0x53494c34u /* "SIL4" */
#define SELINOS_IRQD_REGISTER_MAGIC 0x53495251u /* "SIRQ" */
#define SELINOS_IRQD_REGISTER_ACCEPTED 0x53495241u /* "SIRA" */
#define SELINOS_IRQD_UNREGISTER_MAGIC 0x53495551u /* "SIUQ" */
#define SELINOS_IRQD_UNREGISTER_ACCEPTED 0x53495541u /* "SIUA" */

#endif
