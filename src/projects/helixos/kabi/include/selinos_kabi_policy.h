// SPDX-License-Identifier: MIT
#ifndef SELINOS_KABI_POLICY_H
#define SELINOS_KABI_POLICY_H

#include "selinos_kabi_module.h"

#define SELINOS_KABI_SHA256_BYTES 32U

enum selinos_kabi_policy_status {
    SELINOS_KABI_POLICY_OK = 0,
    SELINOS_KABI_POLICY_E_ARGUMENT = -1,
    SELINOS_KABI_POLICY_E_HASH = -2,
};

/* Freestanding SHA-256 used only for module package integrity policy. */
void selinos_kabi_sha256(const selinos_u8 *data, selinos_size_t size,
                         selinos_u8 digest[SELINOS_KABI_SHA256_BYTES]);

/* Compares a module image against a caller-provided pinned trust digest.
 * This establishes integrity of an exact artifact, not PKCS#7 compatibility,
 * key rotation, distribution signing or execution authorisation. */
int selinos_kabi_check_pinned_digest(const selinos_u8 *image,
                                     selinos_size_t image_size,
                                     const selinos_u8 expected_digest[SELINOS_KABI_SHA256_BYTES]);

#endif
