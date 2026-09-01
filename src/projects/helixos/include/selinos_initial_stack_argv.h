// SPDX-License-Identifier: MIT
#ifndef SELINOS_INITIAL_STACK_ARGV_H
#define SELINOS_INITIAL_STACK_ARGV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal SeLinOS validation contract; not a Linux ABI declaration. */
enum selinos_initial_stack_argv_status {
    SELINOS_INITIAL_STACK_ARGV_FOUND_NUL = 0,
    SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED,
    SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER,
    SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW,
    SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED,
    SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT
};

struct selinos_initial_stack_policy {
    uintptr_t stack_base;
    size_t stack_bytes;
    size_t max_argc;
    size_t max_envc;
    size_t max_string_bytes;
};

struct selinos_initial_stack_argv_result {
    enum selinos_initial_stack_argv_status status;
    size_t length;
    size_t bytes_read;
};

/*
 * Scan one already-authorized argv string for NUL.
 *
 * The caller supplies the authorized mapped region containing argv0. The
 * function never dereferences outside that region or beyond max_string_bytes.
 * A nonzero region_bytes is required; region_bytes and max_string_bytes are
 * independently bounded. If the authorized region ends before the NUL,
 * REGION_EXHAUSTED is returned. The function does not catch hardware page
 * faults.
 */
enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_find_nul(const uint8_t *argv0,
                                    size_t region_bytes,
                                    size_t max_string_bytes,
                                    struct selinos_initial_stack_argv_result *result);

bool selinos_initial_stack_policy_validate(
    const struct selinos_initial_stack_policy *policy);

bool selinos_initial_stack_argv_status_is_success(
    enum selinos_initial_stack_argv_status status);

#ifdef __cplusplus
}
#endif

#endif
