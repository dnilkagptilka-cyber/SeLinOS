#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify the bounded Phase 92 execve-reply argv[0] byte M0."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_execve_reply_argv0_string_byte_m0.verification.json"


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bound_text(record: dict, label: str) -> str:
    path = ROOT / record["path"]
    require(path.is_file(), f"missing {label}")
    require(digest(path) == record["sha256"], f"{label} SHA-256 mismatch")
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    evidence = json.loads(MANIFEST.read_text(encoding="utf-8"))
    require(evidence["schema"] == 1, "invalid schema")
    require(evidence["milestone"] == "SeLinOS execve reply argv0 string-byte M0",
            "wrong milestone")
    require(evidence["status"] == "verified", "milestone is not verified")

    profile = evidence["profile"]
    require(profile["architecture"] == "x86_64/PC99", "wrong architecture")
    require(profile["emulator"] == "QEMU 8.2.2 TCG", "wrong emulator")
    require(profile["runtime_profile"] ==
            "TCG cpu=max, memory=128M, no KVM, bounded 150-second run",
            "wrong runtime profile")
    require(profile["cmake_option"] == "SeLinExecveReplyArgv0StringByteM0=ON",
            "wrong isolated option")
    require(profile["default"] == "OFF", "profile is not default-OFF")
    require(profile["reply"] ==
            "one label-zero 16-word reply through FaultIP; SP and FLAGS excluded",
            "wrong bridge reply contract")
    require(profile["context"] ==
            "root reads and writes one complete faulted context, changing only fixed RIP and RSP",
            "wrong bridge context contract")
    require(profile["witness"] ==
            "one RX mov rax,[rsp+8]; movzx eax,byte ptr [rax]; ud2 reads exactly the first fixed self-authored argv0 byte",
            "wrong first-byte witness contract")

    for label, record in evidence["images"].items():
        path = ROOT / record["path"]
        require(path.is_file() and path.stat().st_size > 0, f"missing or empty {label}")
        require(digest(path) == record["sha256"], f"{label} SHA-256 mismatch")
    config = bound_text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_PROBE" in config,
            "Phase 92 selector disabled")

    sources = {name: bound_text(record, name)
               for name, record in evidence["implementation"].items()}
    bootstrap = sources["bootstrap"]
    require("5387ba9f0b01481fc7027e0883f1c4587c64fd27" in bootstrap,
            "bootstrap does not pin the NXE kernel commit")
    require("EFER.NXE activation" in bootstrap,
            "bootstrap does not describe the NXE dependency")
    lockfile = sources["lockfile"]
    require("5387ba9f0b01481fc7027e0883f1c4587c64fd27  kernel/seL4" in lockfile,
            "source lock does not pin the NXE kernel commit")

    kernel_head = sources["kernel_nxe"]
    for token in ("BEGIN_FUNC(nxe_enable)", "0x80000001", "0x00100000",
                  "IA32_EFER_MSR", "orl $0x800, %eax", "call nxe_enable"):
        require(token in kernel_head, f"missing NXE activation control: {token}")

    cmake = sources["build_gate"]
    for token in ("SeLinExecveReplyArgv0StringByteM0", "DEFAULT\n    OFF",
                  "execve_reply_argv0_string_byte_m0_root.c",
                  "selinos-execve-reply-argv0-pointer-m0-fixture"):
        require(token in cmake, f"missing Phase 92 build control: {token}")
    root_wiring = sources["root_wiring"]
    for token in ("CONFIG_SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_PROBE",
                  "selinos_execve_reply_argv0_string_byte_m0_start",
                  "movzx eax,byte ptr [rax]",
                  "no normal 18-word reply-frame, argv traversal, arbitrary dereference"):
        require(token in root_wiring, f"missing Phase 92 root wiring: {token}")

    public_header = sources["public_header"]
    for token in ("EXPECTED_ARGV0_FIRST_BYTE 0x73u", "POST_READ_UD2_OFFSET 8u",
                  "normal 18-word reply-frame proof", "general execve implementation"):
        require(token in public_header, f"missing Phase 92 header contract: {token}")

    transaction = sources["root_transaction"]
    for token in ("seL4_Fault_UnknownSyscall", "seL4_TCB_ReadRegisters",
                  "seL4_TCB_WriteRegisters", "seL4_UnknownSyscall_FaultIP + 1u",
                  "seL4_Reply(", "seL4_Fault_UserException",
                  "[2u] = 0x44u", "[3u] = 0x24u", "[4u] = 0x08u",
                  "[5u] = 0x0fu", "[6u] = 0xb6u", "[7u] = 0x00u",
                  "[8u] = 0x0fu", "[9u] = 0x0bu",
                  "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_POST_READ_UD2_OFFSET",
                  "SELINOS_EXECVE_REPLY_ARGV0_STRING_BYTE_M0_EXPECTED_ARGV0_FIRST_BYTE"):
        require(token in transaction, f"missing Phase 92 transaction control: {token}")
    require("seL4_UnknownSyscall_SP," not in transaction,
            "Phase 92 must not claim that the normal reply frame restores SP")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "Phase 92 must contain exactly one target resume")

    runtime = bound_text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("not claim" in gate and "normal 18-word" in gate,
            "gate fails to distinguish the normal reply-frame non-claim")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("normal 18-word", "general `execve`", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")
    print("SeLinOS execve reply argv0 string-byte M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
