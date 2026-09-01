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
        "struct selinos_initial_stack_policy",
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
        "SELINOS_INITIAL_STACK_ARGV_FOUND_NUL",
    ):
        require(token in source, f"missing guard token: {token}")

    require("SeLinInitialStackArgvNulM2" in cmake, "missing CMake option")
    require("SELINOS_INITIAL_STACK_ARGV_NUL_M2_PROBE" in cmake, "missing generated selector")
    require("DEFAULT\n    OFF" in cmake, "initial-stack option is not default OFF")
    require("src/selinos_initial_stack_argv.c" in cmake, "checker is not build-integrated")

    for token in (
        "sizeof(terminated)",
        "SELINOS_INITIAL_STACK_ARGV_FOUND_NUL",
        "SELINOS_INITIAL_STACK_ARGV_LIMIT_EXCEEDED",
        "SELINOS_INITIAL_STACK_ARGV_REGION_EXHAUSTED",
        "SELINOS_INITIAL_STACK_ARGV_INVALID_POINTER",
        "selinos_initial_stack_policy_validate",
        "UINTPTR_MAX - 3u",
    ):
        require(token in test, f"missing unit-test case: {token}")

    print("SeLinOS initial-stack argv NUL contract verified.")
    return 0


if __name__ == "__main__":
    main()
