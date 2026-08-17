// SPDX-License-Identifier: MIT
#include <stddef.h>

#include <sel4/sel4.h>

#include "selinos_irqd_protocol.h"

static int registered_irq;
static seL4_Word active_device_token;

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

static void reply_control(seL4_Word value)
{
    seL4_SetMR(0, value);
    seL4_Reply(seL4_MessageInfo_new(0, 0, 0, 1));
}

static void handle_register_request(void)
{
    seL4_Word badge = 0;
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_IRQD_REQUEST_ENDPOINT, &badge);

    if (seL4_MessageInfo_get_length(message) != SELINOS_IRQD_CONTROL_REQUEST_WORDS ||
        seL4_GetMR(0) != SELINOS_IRQD_REGISTER_MAGIC || seL4_GetMR(1) == 0u ||
        seL4_GetMR(2) != active_device_token || registered_irq != 0) {
        reply_control(0u);
        return;
    }

    registered_irq = (int)seL4_GetMR(1);
    reply_control(SELINOS_IRQD_REGISTER_ACCEPTED);
    debug_puts("SeLinOS irqd: driver IRQ registration accepted.\n");
}

static void handle_unregister_request(void)
{
    seL4_Word badge = 0;
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_IRQD_REQUEST_ENDPOINT, &badge);

    if (seL4_MessageInfo_get_length(message) != SELINOS_IRQD_CONTROL_REQUEST_WORDS ||
        seL4_GetMR(0) != SELINOS_IRQD_UNREGISTER_MAGIC || registered_irq == 0 ||
        seL4_GetMR(1) != (seL4_Word)registered_irq ||
        seL4_GetMR(2) != active_device_token) {
        reply_control(0u);
        return;
    }

    if (seL4_CNode_Revoke(SELINOS_IRQD_DRIVER_CNODE_CAP,
                          SELINOS_EDU_DRIVER_NOTIFICATION_CAP,
                          SELINOS_IRQD_DRIVER_CSPACE_BITS) != seL4_NoError) {
        debug_puts("SeLinOS irqd: driver notification revoke failed.\n");
        reply_control(0u);
        return;
    }
    if (seL4_CNode_Delete(SELINOS_IRQD_DRIVER_CNODE_CAP,
                          SELINOS_EDU_DRIVER_NOTIFICATION_CAP,
                          SELINOS_IRQD_DRIVER_CSPACE_BITS) != seL4_NoError) {
        debug_puts("SeLinOS irqd: driver notification delete failed.\n");
        reply_control(0u);
        return;
    }

    registered_irq = 0;
    reply_control(SELINOS_IRQD_UNREGISTER_ACCEPTED);
    debug_puts("SeLinOS irqd: driver notification revoked and registration released.\n");
}

static void handle_hardware_irq(void)
{
    seL4_Word badge = 0;

    if (registered_irq == 0) {
        debug_puts("SeLinOS irqd: IRQ arrived without an active driver registration.\n");
        (void)seL4_IRQHandler_Ack(SELINOS_IRQD_HANDLER_CAP);
        return;
    }

    seL4_Signal(SELINOS_IRQD_DRIVER_NOTIFICATION);
    seL4_MessageInfo_t message = seL4_Recv(SELINOS_IRQD_COMPLETE_ENDPOINT, &badge);
    if (seL4_MessageInfo_get_length(message) != 1u ||
        seL4_GetMR(0) != SELINOS_IRQD_COMPLETE_MAGIC) {
        debug_puts("SeLinOS irqd: rejected malformed IRQ completion; IRQ remains masked.\n");
        return;
    }

    if (seL4_IRQHandler_Ack(SELINOS_IRQD_HANDLER_CAP) != seL4_NoError) {
        debug_puts("SeLinOS irqd: kernel IRQ acknowledgement failed.\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2 || !parse_hex_word(argv[1], &active_device_token) ||
        active_device_token == 0u) {
        debug_puts("SeLinOS irqd: invalid root-issued device token.\n");
        return 1;
    }
    debug_puts("SeLinOS irqd: mediated QEMU edu IRQ service online.\n");

    for (;;) {
        seL4_Word badge = 0;
        (void)seL4_Wait(SELINOS_IRQD_HW_NOTIFICATION_CAP, &badge);

        if ((badge & SELINOS_IRQD_CONTROL_REGISTER_BADGE) != 0u) {
            handle_register_request();
        }
        if ((badge & SELINOS_IRQD_CONTROL_UNREGISTER_BADGE) != 0u) {
            handle_unregister_request();
        }
        if ((badge & (SELINOS_IRQD_CONTROL_REGISTER_BADGE |
                      SELINOS_IRQD_CONTROL_UNREGISTER_BADGE)) == 0u) {
            handle_hardware_irq();
        }
    }
}
