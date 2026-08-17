// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROMFS_PROTOCOL_H
#define SELINOS_ROMFS_PROTOCOL_H

#include <sel4/sel4.h>

/* Both freshly configured processes have their first copied capability at
 * SEL4UTILS_FIRST_FREE (slot 8). Root validates this on startup. */
#define SELINOS_ROMFS_ENDPOINT_SLOT 8u

#define SELINOS_ROMFS_OP_OPEN 1u
#define SELINOS_ROMFS_OP_READ 2u
#define SELINOS_ROMFS_OP_CLOSE 3u
#define SELINOS_ROMFS_OP_OPEN_PATH 4u
#define SELINOS_ROMFS_OP_WRITE 5u

/* OPEN_PATH uses exactly four message registers: opcode, byte length, then
 * two big-endian packed pathname words. It accepts 1..16 non-NUL bytes and
 * does not receive an untrusted pointer or retain client memory. */
#define SELINOS_ROMFS_PATH_MAX_BYTES 16u
#define SELINOS_ROMFS_OPEN_PATH_REQUEST_WORDS 4u

/* File selectors remain a bounded protocol projection. romfsd resolves each
 * selector to an immutable pathname in the embedded CPIO archive; it does not
 * accept an unbounded user-supplied pathname in this M1 parser slice. */
#define SELINOS_ROMFS_FILE_RELEASE 1u
#define SELINOS_ROMFS_FILE_BANNER  2u
#define SELINOS_ROMFS_FILE_VOLATILE_STATE 3u
#define SELINOS_ROMFS_FILE_VERSION 4u
#define SELINOS_ROMFS_FD_RELEASE 3u
#define SELINOS_ROMFS_FD_BANNER  4u
#define SELINOS_ROMFS_FD_VOLATILE_STATE 5u
#define SELINOS_ROMFS_FD_VERSION 6u
#define SELINOS_ROMFS_RELEASE_LENGTH 18u
#define SELINOS_ROMFS_RELEASE_MAGIC 0x53454c494e4f5321ull /* "SELINOS!" */
#define SELINOS_ROMFS_RELEASE_MAGIC_LENGTH 8u
#define SELINOS_ROMFS_BANNER_LENGTH 8u
#define SELINOS_ROMFS_BANNER_MAGIC 0x4350494f2d4f4b21ull /* "CPIO-OK!" */
#define SELINOS_ROMFS_VERSION_LENGTH 8u
#define SELINOS_ROMFS_VERSION_MAGIC 0x53454c494e4f5339ull /* "SELINOS9" */
#define SELINOS_ROMFS_VOLATILE_STATE_LENGTH 8u
#define SELINOS_ROMFS_VOLATILE_STATE_INITIAL 0x424f4f544d302121ull /* "BOOTM0!!" */
#define SELINOS_ROMFS_VOLATILE_STATE_TEST 0x53544154454d3021ull /* "STATEM0!" */

#define SELINOS_ROMFS_STATUS_OK 0u
#define SELINOS_ROMFS_STATUS_EBADF 9u
#define SELINOS_ROMFS_STATUS_ENOENT 2u
#define SELINOS_ROMFS_STATUS_EINVAL 22u

#endif
