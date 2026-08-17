// SPDX-License-Identifier: MIT
#ifndef SELINOS_DMAD_PROTOCOL_H
#define SELINOS_DMAD_PROTOCOL_H

/* `8` is sel4utils' first free CSpace slot. dmad owns the receive endpoint;
 * the driver receives one scoped send/call cap for the pre-granted EDU lease. */
#define SELINOS_PROCESS_FIRST_FREE 8u
#define SELINOS_DMAD_FREE_ENDPOINT (SELINOS_PROCESS_FIRST_FREE)
#define SELINOS_DMAD_DMA_FRAME_CAP (SELINOS_PROCESS_FIRST_FREE + 1u)
#define SELINOS_DMAD_DRIVER_CNODE_CAP (SELINOS_PROCESS_FIRST_FREE + 2u)

/* The custom EDU driver CSpace is 10 bits wide. The DMA child capability is
 * deliberately the exact slot granted to the driver; dmad receives no generic
 * allocator or driver-supplied CSpace pointer. */
#define SELINOS_EDU_DRIVER_CSPACE_BITS 10u
#define SELINOS_EDU_DRIVER_DMA_FRAME_CAP (SELINOS_PROCESS_FIRST_FREE + 256u)
#define SELINOS_EDU_DRIVER_DMA_FREE_ENDPOINT (SELINOS_PROCESS_FIRST_FREE + 260u)

#define SELINOS_DMAD_FREE_MAGIC 0x53444d46u /* "SDMF" */
#define SELINOS_DMAD_FREE_ACCEPTED 0x53444d41u /* "SDMA" */
#define SELINOS_DMAD_FREE_REVOKED  0x53444d52u /* "SDMR" */

#endif
