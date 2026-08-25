// SPDX-License-Identifier: MIT
#ifndef SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_H
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_H

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/*
 * Phase 92 candidate: one fixed execve-number fault is redirected through the
 * Phase 90 context bridge to a self-authored RX witness. The witness reads
 * exactly argv[0] from [rsp+8], then reads only its first byte. This is not a
 * normal 18-word reply-frame proof or a general execve implementation.
 */
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_EXPECTED_ARGV0_FIRST_BYTE 0x73u
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_POST_READ_UD2_OFFSET 8u

bool selinos_execve_reply_argv0_string_byte_m0_start(vka_t *vka, vspace_t *vspace);

#endif
