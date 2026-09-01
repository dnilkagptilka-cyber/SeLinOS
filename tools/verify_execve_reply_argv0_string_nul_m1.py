#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify the bounded Phase 93 argv[0] NUL-sentinel M1."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_execve_reply_argv0_string_nul_m1.verification.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def bind_text(record: dict[str, str], label: str) -> str:
    path = ROOT / record["path"]
    require(path.is_file(), f"missing {label}")
    require(sha256(path) == record["sha256"], f"{label} SHA-256 mismatch")
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    evidence = json.loads(MANIFEST.read_text(encoding="utf-8"))
    require(evidence["schema"] == 1, "invalid manifest schema")
    require(evidence["milestone"] == "SeLinOS execve reply argv0 string-NUL M1", "wrong milestone")
    require(evidence["status"] == "verified", "M1 is not verified")

    profile = evidence["profile"]
    require(profile == {
        "architecture": "x86_64/PC99",
        "emulator": "QEMU 8.2.2 TCG",
        "runtime_profile": "TCG cpu=max, memory=128M, no KVM, bounded 150-second run",
        "cmake_option": "SeLinExecveReplyArgv0StringNulM1=ON",
        "default": "OFF",
        "witness": "mov rax,[rsp+8]; movzx ecx,byte ptr [rax]; movzx edx,byte ptr [rax+7]; ud2",
        "readback": "RAX=0x70002f00 pointer, RCX=0x73 first byte, RDX=0x00 fixed NUL sentinel",
        "reply": "one label-zero 16-word reply through FaultIP; SP and FLAGS excluded",
    }, "unexpected M1 profile")

    for label, record in evidence["images"].items():
        image = ROOT / record["path"]
        require(image.is_file() and image.stat().st_size > 0, f"missing or empty {label}")
        require(sha256(image) == record["sha256"], f"{label} SHA-256 mismatch")
    config = bind_text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_PROBE  1" in config,
            "M1 selector is not enabled")

    implementation = evidence["implementation"]
    header = bind_text(implementation["public_header"], "M1 public header")
    for token in ("EXPECTED_ARGV0_FIRST_BYTE 0x73u", "EXPECTED_ARGV0_NUL_BYTE 0x00u",
                  "EXPECTED_ARGV0_POINTER 0x70002f00u", "FIXED_LENGTH 7u",
                  "POST_READ_UD2_OFFSET 12u", "not a string walk"):
        require(token in header, f"missing header contract: {token}")

    root = bind_text(implementation["root_transaction"], "M1 root transaction")
    for token in ("movzx ecx,byte ptr [rax]", "movzx edx,byte ptr [rax+7]",
                  "[7u] = 0x08u", "[10u] = 0x50u", "[11u] = 0x07u",
                  "observed_context.rax !=",
                  "observed_context.rcx !=",
                  "observed_context.rdx !=",
                  "seL4_TCB_ReadRegisters", "seL4_TCB_WriteRegisters",
                  "seL4_UnknownSyscall_FaultIP + 1u", "seL4_Reply("):
        require(token in root, f"missing M1 transaction control: {token}")
    require(root.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "M1 must contain exactly one target resume")
    require("seL4_UnknownSyscall_SP," not in root,
            "M1 must not claim normal reply-frame SP restoration")

    cmake = bind_text(implementation["build_gate"], "M1 build gate")
    for token in ("SeLinExecveReplyArgv0StringNulM1", "DEFAULT\n    OFF",
                  "execve_reply_argv0_string_nul_m1_root.c"):
        require(token in cmake, f"missing M1 build gate: {token}")
    wiring = bind_text(implementation["root_wiring"], "M1 root wiring")
    for token in ("CONFIG_SELINOS_EXECVE_REPLY_ARGV0_STRING_NUL_M1_PROBE",
                  "selinos_execve_reply_argv0_string_nul_m1_start",
                  "fixed NUL sentinel at argv0+7"):
        require(token in wiring, f"missing M1 root wiring: {token}")
    bootstrap = bind_text(implementation["bootstrap"], "bootstrap")
    require("5387ba9f0b01481fc7027e0883f1c4587c64fd27" in bootstrap,
            "bootstrap lost the NXE kernel pin")

    runtime_record = evidence["runtime_evidence"]
    runtime = bind_text(runtime_record, "M1 runtime transcript")
    for marker in runtime_record["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in runtime_record["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = bind_text(evidence["phase_gate"], "M1 gate")
    require("Status:** VERIFIED" in gate, "M1 gate is not verified")
    for phrase in ("NUL search", "normal 18-word", "general `execve`", "Debian"):
        require(phrase in gate, f"missing M1 non-claim: {phrase}")

    print("SeLinOS execve reply argv0 string-NUL M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
