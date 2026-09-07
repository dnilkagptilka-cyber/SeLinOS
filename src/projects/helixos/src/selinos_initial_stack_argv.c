// SPDX-License-Identifier: MIT
#include "selinos_initial_stack_argv.h"

#include <limits.h>

static void result_reset(struct selinos_initial_stack_argv_result *result)
{
    result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    result->length = 0u;
    result->bytes_read = 0u;
}

static void table_result_reset(struct selinos_initial_stack_argv_table_result *result)
{
    result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    result->argc = 0u;
    result->strings_checked = 0u;
    result->total_bytes_read = 0u;
    result->failure_index = 0u;
}

static bool address_add(uintptr_t base, size_t offset, uintptr_t *address)
{
    if (offset > (size_t)(UINTPTR_MAX - base)) {
        return false;
    }
    *address = base + (uintptr_t)offset;
    return true;
}

static bool reader_read_word(selinos_initial_stack_argv_read_byte_fn reader,
                             void *reader_context,
                             uintptr_t address,
                             uintptr_t *value)
{
    uintptr_t word = 0u;
    size_t byte_index;

    for (byte_index = 0u; byte_index < sizeof(uintptr_t); ++byte_index) {
        uintptr_t byte_address;
        uint8_t byte;

        if (!address_add(address, byte_index, &byte_address) ||
            !reader(reader_context, byte_address, &byte)) {
            return false;
        }
        word |= (uintptr_t)byte << (byte_index * CHAR_BIT);
    }
    *value = word;
    return true;
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
        policy->max_envc == 0u || policy->max_auxv == 0u ||
        policy->max_string_bytes == 0u || policy->page_bytes == 0u) {
        return false;
    }
    if (policy->stack_bytes > (size_t)(UINTPTR_MAX - policy->stack_base)) {
        return false;
    }
    if (policy->max_argc > SIZE_MAX / sizeof(uintptr_t) ||
        policy->max_envc > SIZE_MAX / sizeof(uintptr_t) ||
        policy->max_auxv > SIZE_MAX / 2u) {
        return false;
    }
    return true;
}

static enum selinos_initial_stack_argv_status
parse_table_bounded(uintptr_t table_base,
                    size_t table_region_bytes,
                    size_t argc,
                    size_t count_limit,
                    const struct selinos_initial_stack_policy *policy,
                                       selinos_initial_stack_argv_read_byte_fn reader,
                                       void *reader_context,
                                       struct selinos_initial_stack_argv_table_result *result)
{
    size_t entry_count;
    size_t table_bytes;
    size_t index;

    if (result == NULL) {
        return SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    }
    table_result_reset(result);

    if (policy == NULL || reader == NULL || table_base == 0u ||
        !selinos_initial_stack_policy_validate(policy)) {
        return result->status;
    }
    if (argc > count_limit) {
        result->status = SELINOS_INITIAL_STACK_ARGV_COUNT_LIMIT_EXCEEDED;
        return result->status;
    }
    if (argc == SIZE_MAX) {
        result->status = SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW;
        return result->status;
    }
    entry_count = argc + 1u;
    if (entry_count > SIZE_MAX / sizeof(uintptr_t)) {
        result->status = SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW;
        return result->status;
    }
    table_bytes = entry_count * sizeof(uintptr_t);
    if (table_bytes > table_region_bytes ||
        table_bytes > (size_t)(UINTPTR_MAX - table_base)) {
        result->status = SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED;
        return result->status;
    }

    result->argc = argc;
    for (index = 0u; index < entry_count; ++index) {
        uintptr_t entry_address;
        uintptr_t string_address;
        size_t string_index;
        size_t bytes_read = 0u;
        bool found_nul = false;

        if (!address_add(table_base, index * sizeof(uintptr_t), &entry_address) ||
            !reader_read_word(reader, reader_context, entry_address, &string_address)) {
            result->status = SELINOS_INITIAL_STACK_ARGV_READER_FAULT;
            result->failure_index = index;
            return result->status;
        }
        if (index == argc) {
            if (string_address != 0u) {
                result->status = SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED;
                result->failure_index = index;
                return result->status;
            }
            result->status = SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED;
            return result->status;
        }
        if (string_address == 0u) {
            result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER;
            result->failure_index = index;
            return result->status;
        }

        for (string_index = 0u; string_index < policy->max_string_bytes;
             ++string_index) {
            uintptr_t byte_address;
            uint8_t value;

            if (!policy->allow_cross_page_strings) {
                const size_t page_offset = (size_t)(string_address % policy->page_bytes);
                if (string_index >= policy->page_bytes - page_offset) {
                    result->status = SELINOS_INITIAL_STACK_ARGV_CROSS_PAGE_DISABLED;
                    result->failure_index = index;
                    return result->status;
                }
            }
            if (!address_add(string_address, string_index, &byte_address) ||
                !reader(reader_context, byte_address, &value)) {
                result->status = SELINOS_INITIAL_STACK_ARGV_READER_FAULT;
                result->failure_index = index;
                return result->status;
            }
            bytes_read = string_index + 1u;
            if (value == 0u) {
                found_nul = true;
                break;
            }
        }
        if (!found_nul) {
            result->status = SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED;
            result->failure_index = index;
            return result->status;
        }
        if (result->total_bytes_read > SIZE_MAX - bytes_read) {
            result->status = SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW;
            result->failure_index = index;
            return result->status;
        }
        result->total_bytes_read += bytes_read;
        result->strings_checked = index + 1u;
    }

    result->status = SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED;
    result->failure_index = argc;
    return result->status;
}

enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_parse_table(uintptr_t table_base,
                                       size_t table_region_bytes,
                                       size_t argc,
                                       const struct selinos_initial_stack_policy *policy,
                                       selinos_initial_stack_argv_read_byte_fn reader,
                                       void *reader_context,
                                       struct selinos_initial_stack_argv_table_result *result)
{
    const size_t count_limit = policy == NULL ? 0u : policy->max_argc;

    return parse_table_bounded(table_base, table_region_bytes, argc, count_limit,
                               policy, reader, reader_context, result);
}

enum selinos_initial_stack_argv_status
selinos_initial_stack_envp_parse_table(uintptr_t table_base,
                                       size_t table_region_bytes,
                                       size_t envc,
                                       const struct selinos_initial_stack_policy *policy,
                                       selinos_initial_stack_argv_read_byte_fn reader,
                                       void *reader_context,
                                       struct selinos_initial_stack_envp_table_result *result)
{
    struct selinos_initial_stack_argv_table_result table_result;
    const size_t count_limit = policy == NULL ? 0u : policy->max_envc;
    const enum selinos_initial_stack_argv_status status =
        parse_table_bounded(table_base, table_region_bytes, envc, count_limit,
                            policy, reader, reader_context, &table_result);

    if (result == NULL) {
        return SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    }
    result->status = status;
    result->envc = table_result.argc;
    result->strings_checked = table_result.strings_checked;
    result->total_bytes_read = table_result.total_bytes_read;
    result->failure_index = table_result.failure_index;
    return status;
}

static void auxv_result_reset(struct selinos_initial_stack_auxv_result *result)
{
    result->status = SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    result->entries_checked = 0u;
    result->total_bytes_read = 0u;
    result->failure_index = 0u;
    result->has_type = false;
    result->last_type = 0u;
    result->last_value = 0u;
}

enum selinos_initial_stack_argv_status
selinos_initial_stack_auxv_parse(uintptr_t table_base,
                                 size_t table_region_bytes,
                                 const struct selinos_initial_stack_policy *policy,
                                 selinos_initial_stack_argv_read_byte_fn reader,
                                 void *reader_context,
                                 struct selinos_initial_stack_auxv_result *result)
{
    size_t entry_index;
    const size_t pair_bytes = 2u * sizeof(uintptr_t);

    if (result == NULL) {
        return SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT;
    }
    auxv_result_reset(result);
    if (policy == NULL || reader == NULL || table_base == 0u ||
        !selinos_initial_stack_policy_validate(policy)) {
        return result->status;
    }
    if (table_region_bytes < pair_bytes ||
        pair_bytes > (size_t)(UINTPTR_MAX - table_base)) {
        result->status = SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED;
        return result->status;
    }

    for (entry_index = 0u; entry_index < policy->max_auxv; ++entry_index) {
        uintptr_t entry_address;
        uintptr_t type;
        uintptr_t value;
        uintptr_t value_address;
        size_t offset;

        if (entry_index > SIZE_MAX / pair_bytes) {
            result->status = SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW;
            result->failure_index = entry_index;
            return result->status;
        }
        offset = entry_index * pair_bytes;
        if (offset > table_region_bytes - pair_bytes ||
            !address_add(table_base, offset, &entry_address) ||
            !address_add(entry_address, sizeof(uintptr_t), &value_address) ||
            !reader_read_word(reader, reader_context, entry_address, &type) ||
            !reader_read_word(reader, reader_context, value_address, &value)) {
            result->status = SELINOS_INITIAL_STACK_ARGV_AUXV_READER_FAULT;
            result->failure_index = entry_index;
            return result->status;
        }
        result->entries_checked = entry_index + 1u;
        result->total_bytes_read = result->entries_checked * pair_bytes;
        result->has_type = true;
        result->last_type = type;
        result->last_value = value;
        if (type == 0u) {
            result->status = SELINOS_INITIAL_STACK_ARGV_AUXV_VALIDATED;
            return result->status;
        }
    }

    result->status = SELINOS_INITIAL_STACK_ARGV_AUXV_LIMIT_EXCEEDED;
    result->failure_index = policy->max_auxv;
    return result->status;
}

bool selinos_initial_stack_argv_status_is_success(
    enum selinos_initial_stack_argv_status status)
{
    return status == SELINOS_INITIAL_STACK_ARGV_FOUND_NUL ||
           status == SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED ||
           status == SELINOS_INITIAL_STACK_ARGV_AUXV_VALIDATED;
}
