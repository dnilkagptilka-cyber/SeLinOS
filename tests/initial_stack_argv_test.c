// SPDX-License-Identifier: MIT
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "selinos_initial_stack_argv.h"

struct test_memory {
    uintptr_t base;
    uint8_t bytes[96];
    size_t size;
};

static bool test_read_byte(void *context, uintptr_t address, uint8_t *value)
{
    struct test_memory *memory = context;
    uintptr_t offset;

    if (value == NULL || address < memory->base ||
        address - memory->base >= memory->size) {
        return false;
    }
    offset = address - memory->base;
    *value = memory->bytes[offset];
    return true;
}

static void test_write_word(struct test_memory *memory, size_t offset, uintptr_t value)
{
    size_t byte_index;

    for (byte_index = 0u; byte_index < sizeof(uintptr_t); ++byte_index) {
        memory->bytes[offset + byte_index] =
            (uint8_t)(value >> (byte_index * 8u));
    }
}

int main(void)
{
    static const uint8_t terminated[] = {'s', 'e', 'l', 'i', 'n', 'o', 's', 0u};
    static const uint8_t unterminated[] = {'s', 'e', 'l', 'i', 'n', 'o', 's'};
    struct selinos_initial_stack_argv_result result;
    struct selinos_initial_stack_policy valid_policy = {
        .stack_base = 0x70000000u,
        .stack_bytes = 0x10000u,
        .max_argc = 16u,
        .max_envc = 16u,
        .max_string_bytes = 16u,
        .page_bytes = 8u,
        .allow_cross_page_strings = true,
    };
    struct selinos_initial_stack_policy invalid_policy = valid_policy;
    struct test_memory memory = {
        .base = 0x1000u,
        .size = sizeof(memory.bytes),
    };
    struct selinos_initial_stack_argv_table_result table_result;
    const uintptr_t first_string = 0x103eu;
    const uintptr_t second_string = 0x1048u;
    const uintptr_t env_string = 0x1050u;
    struct selinos_initial_stack_envp_table_result envp_result;

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

    memset(memory.bytes, 0xff, sizeof(memory.bytes));
    memcpy(&memory.bytes[first_string - memory.base], "selinos", 7u);
    memory.bytes[first_string - memory.base + 7u] = 0u;
    memcpy(&memory.bytes[second_string - memory.base], "--m2", 4u);
    memory.bytes[second_string - memory.base + 4u] = 0u;
    test_write_word(&memory, 0u, first_string);
    test_write_word(&memory, sizeof(uintptr_t), second_string);
    test_write_word(&memory, 2u * sizeof(uintptr_t), 0u);
    memcpy(&memory.bytes[env_string - memory.base], "K=V", 3u);
    memory.bytes[env_string - memory.base + 3u] = 0u;
    test_write_word(&memory, 3u * sizeof(uintptr_t), env_string);
    test_write_word(&memory, 4u * sizeof(uintptr_t), 0u);

    assert(selinos_initial_stack_argv_parse_table(memory.base, 3u * sizeof(uintptr_t), 2u,
                                                  &valid_policy, test_read_byte, &memory,
                                                  &table_result) ==
           SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED);
    assert(table_result.argc == 2u);
    assert(table_result.strings_checked == 2u);
    assert(table_result.total_bytes_read == 13u);
    assert(selinos_initial_stack_argv_status_is_success(table_result.status));

    valid_policy.allow_cross_page_strings = false;
    assert(selinos_initial_stack_argv_parse_table(memory.base, 3u * sizeof(uintptr_t), 2u,
                                                  &valid_policy, test_read_byte, &memory,
                                                  &table_result) ==
           SELINOS_INITIAL_STACK_ARGV_CROSS_PAGE_DISABLED);
    assert(table_result.failure_index == 0u);

    valid_policy.allow_cross_page_strings = true;
    assert(selinos_initial_stack_envp_parse_table(memory.base + 3u * sizeof(uintptr_t),
                                                  2u * sizeof(uintptr_t), 1u,
                                                  &valid_policy, test_read_byte, &memory,
                                                  &envp_result) ==
           SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED);
    assert(envp_result.envc == 1u);
    assert(envp_result.strings_checked == 1u);
    assert(envp_result.total_bytes_read == 4u);
    assert(selinos_initial_stack_argv_status_is_success(envp_result.status));
    valid_policy.max_envc = 1u;
    assert(selinos_initial_stack_envp_parse_table(memory.base + 3u * sizeof(uintptr_t),
                                                  2u * sizeof(uintptr_t), 2u,
                                                  &valid_policy, test_read_byte, &memory,
                                                  &envp_result) ==
           SELINOS_INITIAL_STACK_ARGV_COUNT_LIMIT_EXCEEDED);

    test_write_word(&memory, 2u * sizeof(uintptr_t), second_string);
    valid_policy.allow_cross_page_strings = true;
    assert(selinos_initial_stack_argv_parse_table(memory.base, 3u * sizeof(uintptr_t), 2u,
                                                  &valid_policy, test_read_byte, &memory,
                                                  &table_result) ==
           SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED);
    assert(table_result.failure_index == 2u);

    return 0;
}
