// SPDX-License-Identifier: MIT
#include <stddef.h>

#include <sel4/sel4.h>

#include "selinos_dmad_protocol.h"

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

static int parse_hex_word(const char *text, seL4_Word *value)
{
    seL4_Word parsed = 0u;

    if (text == NULL || value == NULL) {
        return 0;
    }
    for (unsigned int i = 0u; text[i] != '\0'; ++i) {
        const char c = text[i];
        seL4_Word digit;
        if (c >= '0' && c <= '9') {
            digit = (seL4_Word)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (seL4_Word)(c - 'a') + 10u;
        } else {
            return 0;
        }
        parsed = (parsed << 4u) | digit;
    }
    *value = parsed;
    return 1;
}

static void reply_rejected(void)
{
    seL4_SetMR(0, 0u);
    seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 1));
}

int main(int argc, char *argv[])
{
    seL4_Word active_device_token;
    int lease_active = 1;

    if (argc != 2 || !parse_hex_word(argv[1], &active_device_token) ||
        active_device_token == 0u) {
        debug_puts("SeLinOS dmad: invalid root-issued device token.\n");
        return 1;
    }
    debug_puts("SeLinOS dmad: QEMU edu DMA lifecycle service online.\n");
    for (;;) {
        seL4_Word badge = 0;
        seL4_MessageInfo_t request = seL4_Recv(SELINOS_DMAD_FREE_ENDPOINT, &badge);
        const seL4_Word length = seL4_MessageInfo_get_length(request);
        const seL4_Word magic = length > 0u ? seL4_GetMR(0) : 0u;
        const seL4_Word device_token = length > 1u ? seL4_GetMR(1) : 0u;
        const seL4_Word dma_address = length > 2u ? seL4_GetMR(2) : 0u;
        const seL4_Word size = length > 3u ? seL4_GetMR(3) : 0u;

        (void)badge;
        if (!lease_active || length != 4u || magic != SELINOS_DMAD_FREE_MAGIC ||
            device_token != active_device_token || dma_address == 0u || size == 0u) {
            reply_rejected();
            continue;
        }

        /* Root created and mapped this exact cap into the driver VSpace, then
         * supplied dmad with a derived mapping cap and the driver CNode. The
         * caller provides no VSpace, CSpace or frame authority. */
        lease_active = 0;
        if (seL4_X86_Page_Unmap(SELINOS_DMAD_DMA_FRAME_CAP) != seL4_NoError) {
            debug_puts("SeLinOS dmad: DMA CPU page unmap failed; lease remains denied.\n");
            reply_rejected();
            continue;
        }
        if (seL4_CNode_Revoke(SELINOS_DMAD_DRIVER_CNODE_CAP,
                              SELINOS_EDU_DRIVER_DMA_FRAME_CAP,
                              SELINOS_EDU_DRIVER_CSPACE_BITS) != seL4_NoError) {
            debug_puts("SeLinOS dmad: DMA child capability revoke failed; lease remains denied.\n");
            reply_rejected();
            continue;
        }
        if (seL4_CNode_Delete(SELINOS_DMAD_DRIVER_CNODE_CAP,
                              SELINOS_EDU_DRIVER_DMA_FRAME_CAP,
                              SELINOS_EDU_DRIVER_CSPACE_BITS) != seL4_NoError) {
            debug_puts("SeLinOS dmad: DMA child capability delete failed; lease remains denied.\n");
            reply_rejected();
            continue;
        }

        seL4_SetMR(0, SELINOS_DMAD_FREE_ACCEPTED);
        seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 1));
        debug_puts("SeLinOS dmad: DMA CPU mapping unmapped and child capability revoked.\n");
    }
}
