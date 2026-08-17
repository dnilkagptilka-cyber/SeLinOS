// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_romfs_protocol.h"

static seL4_MessageInfo_t call_romfs(seL4_Word operation, seL4_Word first,
                                     seL4_Word second, seL4_Word length)
{
    seL4_SetMR(0, operation);
    if (length > 1u) {
        seL4_SetMR(1, first);
    }
    if (length > 2u) {
        seL4_SetMR(2, second);
    }
    return seL4_Call(SELINOS_ROMFS_ENDPOINT_SLOT,
                     seL4_MessageInfo_new(0u, 0u, 0u, length));
}

static seL4_MessageInfo_t call_romfs_open_path(seL4_Word byte_length,
                                                  seL4_Word first_word,
                                                  seL4_Word second_word)
{
    seL4_SetMR(0, SELINOS_ROMFS_OP_OPEN_PATH);
    seL4_SetMR(1, byte_length);
    seL4_SetMR(2, first_word);
    seL4_SetMR(3, second_word);
    return seL4_Call(SELINOS_ROMFS_ENDPOINT_SLOT,
                     seL4_MessageInfo_new(0u, 0u, 0u,
                                          SELINOS_ROMFS_OPEN_PATH_REQUEST_WORDS));
}

static int reply_is(seL4_MessageInfo_t reply, seL4_Word status, seL4_Word value)
{
    return seL4_MessageInfo_get_label(reply) == 0u &&
           seL4_MessageInfo_get_length(reply) >= 2u &&
           seL4_GetMR(0) == status && seL4_GetMR(1) == value;
}

int main(void)
{
    seL4_MessageInfo_t reply = call_romfs(SELINOS_ROMFS_OP_OPEN,
                                          SELINOS_ROMFS_FILE_RELEASE, 0u, 2u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_FD_RELEASE)) {
        seL4_DebugPutString("SeLinOS ROMFS probe: open failed.\n");
        return 1;
    }

    reply = call_romfs(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_RELEASE, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_RELEASE_LENGTH) ||
        seL4_MessageInfo_get_length(reply) != 3u ||
        seL4_GetMR(2) != SELINOS_ROMFS_RELEASE_MAGIC) {
        seL4_DebugPutString("SeLinOS ROMFS probe: immutable read failed.\n");
        return 1;
    }

    reply = call_romfs(SELINOS_ROMFS_OP_READ, 99u, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_EBADF, 0u)) {
        seL4_DebugPutString("SeLinOS ROMFS probe: invalid-FD guard failed.\n");
        return 1;
    }

    reply = call_romfs(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_RELEASE, 0u, 2u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, 0u)) {
        seL4_DebugPutString("SeLinOS ROMFS probe: release close failed.\n");
        return 1;
    }

    reply = call_romfs_open_path(15u, 0x2f73656c696e6f73ull,
                                 0x2d62616e6e657200ull); /* "/selinos-banner" */
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_FD_BANNER)) {
        seL4_DebugPutString("SeLinOS ROMFS probe: CPIO banner open failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_BANNER, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_BANNER_LENGTH) ||
        seL4_MessageInfo_get_length(reply) != 3u ||
        seL4_GetMR(2) != SELINOS_ROMFS_BANNER_MAGIC) {
        seL4_DebugPutString("SeLinOS ROMFS probe: CPIO banner read failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_BANNER, 0u, 2u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, 0u)) {
        seL4_DebugPutString("SeLinOS ROMFS probe: CPIO banner close failed.\n");
        return 1;
    }

    reply = call_romfs_open_path(14u, 0x2f73656c696e6f73ull,
                                 0x2d73746174650000ull); /* "/selinos-state" */
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_FD_VOLATILE_STATE)) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: volatile state open failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_VOLATILE_STATE_LENGTH) ||
        seL4_MessageInfo_get_length(reply) != 3u ||
        seL4_GetMR(2) != SELINOS_ROMFS_VOLATILE_STATE_INITIAL) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: volatile initial read failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_WRITE, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 2u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_EBADF, 0u)) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: malformed volatile write was accepted.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_VOLATILE_STATE_LENGTH) ||
        seL4_MessageInfo_get_length(reply) != 3u ||
        seL4_GetMR(2) != SELINOS_ROMFS_VOLATILE_STATE_INITIAL) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: malformed write changed volatile state.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_WRITE, SELINOS_ROMFS_FD_VOLATILE_STATE,
                       SELINOS_ROMFS_VOLATILE_STATE_TEST, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_VOLATILE_STATE_LENGTH)) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: volatile write failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_VOLATILE_STATE_LENGTH) ||
        seL4_MessageInfo_get_length(reply) != 3u ||
        seL4_GetMR(2) != SELINOS_ROMFS_VOLATILE_STATE_TEST) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: volatile read-after-write failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 2u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_OK, 0u)) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: volatile close failed.\n");
        return 1;
    }
    reply = call_romfs(SELINOS_ROMFS_OP_WRITE, SELINOS_ROMFS_FD_VOLATILE_STATE,
                       SELINOS_ROMFS_VOLATILE_STATE_INITIAL, 3u);
    if (!reply_is(reply, SELINOS_ROMFS_STATUS_EBADF, 0u)) {
        seL4_DebugPutString("SeLinOS VFS M0 probe: closed volatile FD accepted write.\n");
        return 1;
    }

    seL4_DebugPutString("SeLinOS ROMFS probe: open/read/close and invalid-FD guard passed.\n");
    seL4_DebugPutString("SeLinOS ROMFS probe: CPIO pathname multi-file lookup passed.\n");
    seL4_DebugPutString("SeLinOS ROMFS probe: bounded packed client pathname lookup passed.\n");
    seL4_DebugPutString("SeLinOS VFS M0 probe: bounded volatile write/read/close lifecycle passed.\n");
    for (;;) {
        (void)seL4_Yield();
    }
}
