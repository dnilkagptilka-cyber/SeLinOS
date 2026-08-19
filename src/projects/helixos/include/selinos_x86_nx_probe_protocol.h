// SPDX-License-Identifier: MIT
#pragma once

#include <sel4/sel4.h>

/* Fixed, root-selected test mapping. It is not an ELF segment or an ABI page. */
#define SELINOS_X86_NX_PROBE_VADDR ((seL4_Word)0x70000000u)
#define SELINOS_X86_NX_PROBE_PAGE_BYTES (1u << seL4_PageBits)
#define SELINOS_X86_NX_PROBE_SUCCESS_ENDPOINT_SLOT ((seL4_CPtr)8u)
#define SELINOS_X86_NX_PROBE_SUCCESS_MAGIC ((seL4_Word)0x4e58455845434f4bull)
#define SELINOS_X86_NX_PROBE_SUCCESS_WORDS 1u

/* The root writes only this single `ret` byte into a freshly allocated page.
 * The executable control child returns normally; the execute-disabled child
 * must fault before it can execute this byte. */
#define SELINOS_X86_NX_PROBE_RET_OPCODE 0xc3u
/* x86 page-fault error code: present + user + instruction fetch.  A set
 * reserved-bit flag would be a different fault class and is rejected. */
#define SELINOS_X86_NX_PROBE_EXECUTE_DISABLE_FSR 0x15u
