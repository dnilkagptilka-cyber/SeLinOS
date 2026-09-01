// SPDX-License-Identifier: MIT
#include "selinos_initial_stack_argv.h"

#include <limits.h>

static void result_reset(struct selinos_initial_stack_argv_result *result)
{
    result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    result->length = 0u;
    result->bytes_read = 0u;
}

enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_find_nul(const uint8_t *argv0,
                                    size_t region_bytes,
                                    size_t max_string_bytes,
                                    struct selinos_initial_stack_argv_result *result)
{
    uintptr_t base;
    size_t limit;
    size_t index;

    if (result == NULL) {
        return SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    }
    result_reset(result);

    if (argv0 == NULL) {
        result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER;
        return result->status;
    }
    if (region_bytes == 0u) {
        return result->status;
    }
    if (max_string_bytes == 0u) {
        result->status = SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED;
        return result->status;
    }

    base = (uintptr_t)argv0;
    if (region_bytes > (size_t)(UINTPTR_MAX - base)) {
        result->status = SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW;
        return result->status;
    }

    limit = region_bytes < max_string_bytes ? region_bytes : max_string_bytes;
    for (index = 0u; index < limit; ++index) {
        const uintptr_t address = base + (uintptr_t)index;
        const uint8_t value = *(const uint8_t *)address;

        result->bytes_read = index + 1u;
        if (value == 0u) {
            result->status = SELINOS_INITIAL_STACK_ARGV_FOUND_NUL;
            result->length = index;
            return result->status;
        }
    }

    result->status = limit < max_string_bytes
                         ? SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED
                         : SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED;
    return result->status;
}

bool selinos_initial_stack_policy_validate(
    const struct selinos_initial_stack_policy *policy)
{
    if (policy == NULL || policy->stack_bytes == 0u || policy->max_argc == 0u ||
        policy->max_envc == 0u || policy->max_string_bytes == 0u) {
        return false;
    }
    if (policy->stack_bytes > (size_t)(UINTPTR_MAX - policy->stack_base)) {
        return false;
    }
    if (policy->max_argc > SIZE_MAX / sizeof(uintptr_t) ||
        policy->max_envc > SIZE_MAX / sizeof(uintptr_t)) {
        return false;
    }
    return true;
}

bool selinos_initial_stack_argv_status_is_success(
    enum selinos_initial_stack_argv_status status)
{
    return status == SELINOS_INITIAL_STACK_ARGV_FOUND_NUL;
}
