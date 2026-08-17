// SPDX-License-Identifier: MIT
#ifndef SELINOS_DEVICED_PROTOCOL_H
#define SELINOS_DEVICED_PROTOCOL_H

/*
 * Profile-limited PCI registration contract for the QEMU edu reference device.
 * The endpoint capability itself is the non-forgeable authority to request a
 * match. Driver domains receive neither PCI config-space I/O nor a physical
 * resource allocator. `deviced` retains the authoritative device record and
 * returns only an immutable shadow-object description suitable for the local
 * KAPI shim to invoke the driver's probe callback in its own VSpace.
 */
#define SELINOS_DEVICED_REQUEST_ENDPOINT 8u

/* The edu driver CSpace first maps BAR0 in slots 8..263 and receives the M1
 * IRQ/DMA capabilities in slots 264..270. `deviced` is deliberately next. */
#define SELINOS_EDU_DRIVER_DEVICED_ENDPOINT 271u

#define SELINOS_DEVICED_REGISTER_MAGIC    0x53445052u /* "SDPR" */
#define SELINOS_DEVICED_REGISTER_ACCEPTED 0x53445041u /* "SDPA" */
#define SELINOS_DEVICED_REGISTER_NO_MATCH 0x5344504eu /* "SDPN" */

#define SELINOS_DEVICED_QEMU_EDU_VENDOR 0x1234u
#define SELINOS_DEVICED_QEMU_EDU_DEVICE 0x11e8u
/* `deviced` returns a non-zero root-issued opaque token. The value identifies
 * one profile-approved device generation, but it is not a capability and does
 * not confer any PCI, memory or IRQ authority by itself. */

/* Register request: magic, vendor, device. */
#define SELINOS_DEVICED_REGISTER_REQUEST_WORDS 3u

/* Accepted reply: magic, device token, vendor, device, IRQ, BAR0 start,
 * BAR0 length. Every datum describes the one profile-approved resource. */
#define SELINOS_DEVICED_REGISTER_REPLY_WORDS 7u
#define SELINOS_DEVICED_REPLY_MAGIC_INDEX 0u
#define SELINOS_DEVICED_REPLY_TOKEN_INDEX 1u
#define SELINOS_DEVICED_REPLY_VENDOR_INDEX 2u
#define SELINOS_DEVICED_REPLY_DEVICE_INDEX 3u
#define SELINOS_DEVICED_REPLY_IRQ_INDEX 4u
#define SELINOS_DEVICED_REPLY_BAR0_START_INDEX 5u
#define SELINOS_DEVICED_REPLY_BAR0_LEN_INDEX 6u

#endif
