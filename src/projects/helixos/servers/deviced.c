// SPDX-License-Identifier: MIT
#include <stddef.h>

#include <sel4/sel4.h>

#include "selinos_deviced_protocol.h"

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

static int parse_hex_word(const char *text, seL4_Word *value)
{
    seL4_Word parsed = 0;

    if (text == NULL || value == NULL) {
        return 0;
    }
    for (unsigned int i = 0; text[i] != '\0'; ++i) {
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

static int request_matches_qemu_edu(seL4_Word vendor, seL4_Word device)
{
    return (vendor == SELINOS_DEVICED_QEMU_EDU_VENDOR || vendor == 0xffffu) &&
           (device == SELINOS_DEVICED_QEMU_EDU_DEVICE || device == 0xffffu);
}

int main(int argc, char *argv[])
{
    seL4_Word bar0_start;
    seL4_Word bar0_len;
    seL4_Word irq;
    seL4_Word device_token;

    if (argc == 1) {
        debug_puts("SeLinOS deviced: no PCI device record was issued; registry dormant.\n");
        for (;;) {
            (void)seL4_Yield();
        }
    }
    if (argc != 5 || !parse_hex_word(argv[1], &bar0_start) ||
        !parse_hex_word(argv[2], &bar0_len) || !parse_hex_word(argv[3], &irq) ||
        !parse_hex_word(argv[4], &device_token) || bar0_len == 0u || irq == 0u ||
        irq > 0xffu || device_token == 0u) {
        debug_puts("SeLinOS deviced: invalid root-issued QEMU edu resource record.\n");
        return 1;
    }

    debug_puts("SeLinOS deviced: mediated QEMU edu PCI registry online.\n");
    for (;;) {
        seL4_Word badge = 0;
        seL4_MessageInfo_t request = seL4_Recv(SELINOS_DEVICED_REQUEST_ENDPOINT, &badge);
        const seL4_Word length = seL4_MessageInfo_get_length(request);
        const seL4_Word magic = length > 0u ? seL4_GetMR(0) : 0u;
        const seL4_Word vendor = length > 1u ? seL4_GetMR(1) : 0u;
        const seL4_Word device = length > 2u ? seL4_GetMR(2) : 0u;

        (void)badge;
        if (length != SELINOS_DEVICED_REGISTER_REQUEST_WORDS ||
            magic != SELINOS_DEVICED_REGISTER_MAGIC) {
            seL4_SetMR(0, SELINOS_DEVICED_REGISTER_NO_MATCH);
            seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 1));
            continue;
        }
        if (!request_matches_qemu_edu(vendor, device)) {
            debug_puts("SeLinOS deviced: PCI driver registration had no matching device.\n");
            seL4_SetMR(0, SELINOS_DEVICED_REGISTER_NO_MATCH);
            seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 1));
            continue;
        }

        seL4_SetMR(0, SELINOS_DEVICED_REGISTER_ACCEPTED);
        seL4_SetMR(1, device_token);
        seL4_SetMR(2, SELINOS_DEVICED_QEMU_EDU_VENDOR);
        seL4_SetMR(3, SELINOS_DEVICED_QEMU_EDU_DEVICE);
        seL4_SetMR(4, irq);
        seL4_SetMR(5, bar0_start);
        seL4_SetMR(6, bar0_len);
        seL4_Reply(seL4_MessageInfo_new(0, 0, 0,
                                        SELINOS_DEVICED_REGISTER_REPLY_WORDS));
        debug_puts("SeLinOS deviced: QEMU edu PCI ID matched; driver-local probe authorised.\n");
    }
}
