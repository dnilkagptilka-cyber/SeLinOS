#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check the static contract of the unverified Phase 92 argv[0] byte witness."""
from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def source(relative: str) -> str:
    path = ROOT / relative
    require(path.is_file(), f"missing source: {relative}")
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    cmake = source("src/projects/helixos/CMakeLists.txt")
    wiring = source("src/projects/helixos/src/domain_manager.c")
    header = source("src/projects/helixos/include/selinos_execve_reply_argv0_string_byte_m0.h")
    transaction = source("src/projects/helixos/src/execve_reply_argv0_string_byte_m0_root.c")

    for token in (
        "SeLinExecveReplyArgv0StringByteM0",
        "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_PROBE",
        "Enable one controlled execve reply-context bridge argv0 first-byte read candidate",
        "DEFAULT\n    OFF",
        "execve_reply_argv0_string_byte_m0_root.c",
        "selinos-execve-reply-argv0-pointer-m0-fixture",
    ):
        require(token in cmake, f"missing Phase 92 build control: {token}")

    for token in (
        "CONFIG_SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_PROBE",
        "selinos_execve_reply_argv0_string_byte_m0_start",
        "movzx eax,byte ptr [rax]",
        "no normal 18-word reply-frame, argv traversal, arbitrary dereference",
    ):
        require(token in wiring, f"missing Phase 92 root wiring: {token}")

    for token in (
        "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_EXPECTED_ARGV0_FIRST_BYTE 0x73u",
        "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_POST_READ_UD2_OFFSET 8u",
        "normal 18-word reply-frame proof",
        "general execve implementation",
    ):
        require(token in header, f"missing Phase 92 header contract: {token}")

    for token in (
        "selinos_execve_reply_argv0_pointer_m0_fixture",
        "seL4_Fault_UnknownSyscall",
        "seL4_TCB_ReadRegisters",
        "seL4_TCB_WriteRegisters",
        "seL4_Reply(",
        "seL4_Fault_UserException",
        "[2u] = 0x44u",
        "[3u] = 0x24u",
        "[4u] = 0x08u",
        "[5u] = 0x0fu",
        "[6u] = 0xb6u",
        "[7u] = 0x00u",
        "[8u] = 0x0fu",
        "[9u] = 0x0bu",
        "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_POST_READ_UD2_OFFSET",
        "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_EXPECTED_ARGV0_FIRST_BYTE",
    ):
        require(token in transaction, f"missing Phase 92 transaction control: {token}")
    require("seL4_UnknownSyscall_SP," not in transaction,
            "Phase 92 must not claim that the normal reply frame restores SP")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "Phase 92 must contain exactly one target resume")
    require(transaction.count("0x0fu") == 2,
            "Phase 92 witness must contain exactly movzx and UD2 opcode prefixes")

    print("SeLinOS Phase 92 argv0 first-byte candidate static contract verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
