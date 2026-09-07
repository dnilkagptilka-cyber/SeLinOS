#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the isolated Phase 98 runtime auxv witness contract."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> int:
    cmake = text("src/projects/helixos/CMakeLists.txt")
    main_source = text("src/projects/helixos/src/main.c")
    header = text("src/projects/helixos/include/selinos_initial_stack_auxv_runtime_m6.h")
    source = text("src/projects/helixos/src/selinos_initial_stack_auxv_runtime_m6.c")
    require("SeLinInitialStackAuxvRuntimeM6" in cmake, "missing M6 CMake option")
    require("SELINOS_INITIAL_STACK_AUXV_RUNTIME_M6_PROBE" in cmake, "missing M6 selector")
    require("DEFAULT\n    OFF" in cmake, "M6 must be default OFF")
    require("selinos_initial_stack_argv.c" in cmake and
            "selinos_initial_stack_auxv_runtime_m6.c" in cmake,
            "M6 must link parser and runtime witness")
    require("CONFIG_SELINOS_INITIAL_STACK_AUXV_RUNTIME_M6_PROBE" in main_source,
            "main must compile-guard M6 witness")
    for token in (
        "selinos_initial_stack_auxv_runtime_m6_witness",
        "selinos_initial_stack_auxv_parse",
        "read_authorized_byte",
        "address < reader->base",
        "offset >= reader->length",
        "max_auxv = 4u",
        "3u, 0x00400040u",
        "9u, 0x00401000u",
        "0u, 0u",
        "SELINOS_INITIAL_STACK_ARGV_AUXV_VALIDATED",
        "entries_checked != 3u",
        "SeLinOS Phase 98 M6: auxv runtime witness validated.",
    ):
        require(token in source, f"missing M6 guard/evidence token: {token}")
    require("not a Debian ABI implementation" in header or
            "not a Debian ABI implementation" in source,
            "M6 must state its compatibility boundary")
    print("SeLinOS Phase 98 M6 runtime auxv witness contract verified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
