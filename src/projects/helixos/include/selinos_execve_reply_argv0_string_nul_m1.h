// SPDX-License-Identifier: MIT
#ifndef SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_H
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_H

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/*
 * Phase 93 M1: one fixed execve-number fault is redirected through the
 * Phase 90 context bridge to a self-authored RX witness. The witness loads
 * argv[0] from [rsp+8], reads its first byte, then reads one fixed NUL
 * sentinel at argv[0]+7. This is not a string walk, normal 18-word
 * reply-frame proof, or general execve implementation.
 */
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_EXPECTED_ARGV0_FIRST_BYTE 0x73u
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_EXPECTED_ARGV0_NUL_BYTE 0x00u
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_EXPECTED_ARGV0_POINTER 0x70002f00u
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_FIXED_LENGTH 7u
#define SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_POST_READ_UD2_OFFSET 12u

bool selinos_execve_reply_argv0_string_nul_m1_start(vka_t *vka, vspace_t *vspace);

#endif
