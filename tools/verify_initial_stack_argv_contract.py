#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the first initial-stack argv contract and bounded NUL checker."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> int:
    header = text("src/projects/helixos/include/selinos_initial_stack_argv.h")
    source = text("src/projects/helixos/src/selinos_initial_stack_argv.c")
    cmake = text("src/projects/helixos/CMakeLists.txt")
    test = text("tests/initial_stack_argv_test.c")

    for token in (
        "SELINOS_INITIAL_STACK_ARGV_FOUND_NUL",
        "SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED",
        "SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER",
        "SELINOS_INITIAL_STACK_ARGV_ARITHMETIC_OVERFLOW",
        "SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED",
        "SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED",
        "SELINOS_INITIAL_STACK_ARGV_COUNT_LIMIT_EXCEEDED",
        "SELINOS_INITIAL_STACK_ARGV_READER_FAULT",
        "SELINOS_INITIAL_STACK_ARGV_CROSS_PAGE_DISABLED",
        "SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED",
        "struct selinos_initial_stack_policy",
        "struct selinos_initial_stack_argv_table_result",
        "selinos_initial_stack_argv_parse_table",
        "selinos_initial_stack_argv_read_byte_fn",
        "selinos_initial_stack_policy_validate",
        "selinos_initial_stack_argv_find_nul",
        "max_string_bytes",
        "not a Linux ABI declaration",
    ):
        require(token in header, f"missing contract token: {token}")

    for token in (
        "argv0 == NULL",
        "region_bytes == 0u",
        "max_string_bytes == 0u",
        "UINTPTR_MAX - base",
        "base + (uintptr_t)index",
        "value == 0u",
        "limit < max_string_bytes",
        "SIZE_MAX / sizeof(uintptr_t)",
        "reader_read_word",
        "entry_count = argc + 1u",
        "table_bytes = entry_count * sizeof(uintptr_t)",
        "string_address == 0u",
        "policy->allow_cross_page_strings",
        "SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED",
        "SELINOS_INITIAL_STACK_ARGV_FOUND_NUL",
    ):
        require(token in source, f"missing guard token: {token}")

    require("SeLinInitialStackArgvNulM2" in cmake, "missing M2 CMake option")
    require("SeLinInitialStackArgvTableM3" in cmake, "missing M3 CMake option")
    require("SELINOS_INITIAL_STACK_ARGV_TABLE_M3_PROBE" in cmake, "missing M3 selector")
    require(cmake.count("DEFAULT\n    OFF") >= 2, "initial-stack options are not default OFF")
    require("SeLinInitialStackArgvNulM2 OR SeLinInitialStackArgvTableM3" in cmake,
            "checker is not build-integrated for M2/M3")

    for token in (
        "sizeof(terminated)",
        "SELINOS_INITIAL_STACK_ARGV_FOUND_NUL",
        "SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED",
        "SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED",
        "SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER",
        "selinos_initial_stack_policy_validate",
        "UINTPTR_MAX - 3u",
        "allow_cross_page_strings = true",
        "SELINOS_INITIAL_STACK_ARGV_TABLE_VALIDATED",
        "SELINOS_INITIAL_STACK_ARGV_CROSS_PAGE_DISABLED",
        "SELINOS_INITIAL_STACK_ARGV_TABLE_NOT_TERMINATED",
        "test_read_byte",
    ):
        require(token in test, f"missing unit-test case: {token}")

    print("SeLinOS initial-stack argv table and cross-page NUL contract verified.")
    return 0


if __name__ == "__main__":
    main()
