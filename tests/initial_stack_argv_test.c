// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdint.h>

#include "selinos_initial_stack_argv.h"

int main(void)
{
    static const uint8_t terminated[] = {'s', 'e', 'l', 'i', 'n', 'o', 's', 0u};
    static const uint8_t unterminated[] = {'s', 'e', 'l', 'i', 'n', 'o', 's'};
    struct selinos_initial_stack_argv_result result;
    const struct selinos_initial_stack_policy valid_policy = {
        .stack_base = 0x70000000u,
        .stack_bytes = 0x10000u,
        .max_argc = 16u,
        .max_envc = 16u,
        .max_string_bytes = 4096u,
    };
    struct selinos_initial_stack_policy invalid_policy = valid_policy;

    assert(selinos_initial_stack_policy_validate(&valid_policy));
    invalid_policy.max_argc = 0u;
    assert(!selinos_initial_stack_policy_validate(&invalid_policy));
    invalid_policy = valid_policy;
    invalid_policy.stack_base = UINTPTR_MAX - 3u;
    invalid_policy.stack_bytes = 8u;
    assert(!selinos_initial_stack_policy_validate(&invalid_policy));

    assert(selinos_initial_stack_argv_find_nul(terminated, sizeof(terminated), 16u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_FOUND_NUL);
    assert(result.length == 7u);
    assert(result.bytes_read == 8u);
    assert(selinos_initial_stack_argv_status_is_success(result.status));

    assert(selinos_initial_stack_argv_find_nul(terminated, sizeof(terminated), 7u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED);
    assert(result.length == 0u);
    assert(result.bytes_read == 7u);

    assert(selinos_initial_stack_argv_find_nul(unterminated, sizeof(unterminated), 16u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED);
    assert(result.bytes_read == 7u);

    assert(selinos_initial_stack_argv_find_nul(NULL, sizeof(terminated), 16u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER);
    assert(selinos_initial_stack_argv_find_nul(terminated, sizeof(terminated), 0u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED);
    assert(selinos_initial_stack_argv_find_nul(terminated, 0u, 16u,
                                               &result) == SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT);
    assert(selinos_initial_stack_argv_find_nul(terminated, sizeof(terminated), 16u, NULL) ==
           SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT);

    return 0;
}
