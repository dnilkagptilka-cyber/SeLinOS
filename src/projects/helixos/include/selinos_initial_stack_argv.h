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
    SELINOS_INITIAL_STACK_ARGV_INVALID_ARGUMENT,
    SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED,
    SELINOS_INITIAL_STACK_ARGV_COUNT_LIMIT_EXCEEDED,
    SELINOS_INITIAL_STACK_ARGV_READER_FAULT,
    SELINOS_INITIAL_STACK_ARGV_CROSS_PAGE_DISABLED,
    SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED
};

struct selinos_initial_stack_policy {
    uintptr_t stack_base;
    size_t stack_bytes;
    size_t max_argc;
    size_t max_envc;
    size_t max_string_bytes;
    size_t page_bytes;
    bool allow_cross_page_strings;
};

struct selinos_initial_stack_argv_result {
    enum selinos_initial_stack_argv_status status;
    size_t length;
    size_t bytes_read;
};

struct selinos_initial_stack_argv_table_result {
    enum selinos_initial_stack_argv_status status;
    size_t argc;
    size_t strings_checked;
    size_t total_bytes_read;
    size_t failure_index;
};

/* Reader owns the authority boundary and may map/validate one byte at a time. */
typedef bool (*selinos_initial_stack_argv_read_byte_fn)(void *context,
                                                        uintptr_t address,
                                                        uint8_t *value);

/* Scan one already-authorized argv string for NUL. */
enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_find_nul(const uint8_t *argv0,
                                    size_t region_bytes,
                                    size_t max_string_bytes,
                                    struct selinos_initial_stack_argv_result *result);

/*
 * Parse argc pointer entries plus argv[argc] == NULL through an authorized
 * reader. The reader is used for both the pointer table and all string bytes,
 * so strings may cross pages without unchecked host dereferences. The parser
 * never remaps pages and does not catch hardware faults in an unsafe reader.
 */
enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_parse_table(uintptr_t table_base,
                                       size_t table_region_bytes,
                                       size_t argc,
                                       const struct selinos_initial_stack_policy *policy,
                                       selinos_initial_stack_argv_read_byte_fn reader,
                                       void *reader_context,
                                       struct selinos_initial_stack_argv_table_result *result);

bool selinos_initial_stack_policy_validate(
    const struct selinos_initial_stack_policy *policy);

bool selinos_initial_stack_argv_status_is_success(
    enum selinos_initial_stack_argv_status status);

#ifdef __cplusplus
}
#endif

#endif
