// SPDX-License-Identifier: MIT
#ifndef SELINOS_ROOT_CONSTRUCT_M1_PROTOCOL_H
#define SELINOS_ROOT_CONSTRUCT_M1_PROTOCOL_H

#include <sel4/sel4.h>

/* Phase 35 M1: opt-in root-held, one-slot fixed-child construction proof.
 * The protocol deliberately contains no pointer, image, stack, register, TLS,
 * clone, scheduler, credential, VFS or device field. */
#define SELINOS_ROOT_CONSTRUCT_M1_REQUEST 0x52434d3152455154ull /* "RCM1REQT" */
#define SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED 0x52434d31434e5354ull /* "RCM1CNST" */
#define SELINOS_ROOT_CONSTRUCT_M1_REJECTED 0x52434d3152454a43ull /* "RCM1REJC" */
#define SELINOS_ROOT_CONSTRUCT_M1_SLOT 1u
#define SELINOS_ROOT_CONSTRUCT_M1_GENERATION 1u
#define SELINOS_ROOT_CONSTRUCT_M1_REQUEST_WORDS 3u
#define SELINOS_ROOT_CONSTRUCT_M1_REPLY_WORDS 3u

/* The adapter client receives only this endpoint send cap. Root keeps the
 * receive cap private and never transfers the constructed child bundle in M1. */
#define SELINOS_ROOT_CONSTRUCT_M1_TASKD_REQUEST_ENDPOINT_SLOT 8u

#endif
