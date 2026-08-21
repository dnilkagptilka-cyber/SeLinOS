#!/usr/bin/env python3
"""Independently verify the bounded Phase 90 execve reply argv0-pointer M0."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_execve_reply_argv0_pointer_m0.verification.json"


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bound_text(record: dict, label: str) -> str:
    path = ROOT / record["path"]
    require(path.is_file(), f"missing {label}")
    require(digest(path) == record["sha256"], f"{label} SHA-256 mismatch")
    return path.read_text(errors="replace")


def main() -> int:
    evidence = json.loads(MANIFEST.read_text())
    require(evidence["schema"] == 1, "invalid schema")
    require(evidence["milestone"] == "SeLinOS execve reply argv0-pointer M0",
            "wrong milestone")
    require(evidence["status"] == "verified", "milestone is not verified")
    profile = evidence["profile"]
    require(profile["architecture"] == "x86_64/PC99", "wrong architecture")
    require(profile["cmake_option"] == "SeLinExecveReplyArgv0PointerM0=ON",
            "wrong isolated option")
    require(profile["reply"] ==
            "one label-zero 16-word reply through FaultIP; SP and FLAGS excluded",
            "wrong bridge reply contract")
    require(profile["context"] ==
            "root reads and writes one complete faulted context, changing only fixed RIP and RSP",
            "wrong bridge context contract")
    require(profile["witness"] ==
            "one RX mov rax,[rsp+8]; ud2 reads exactly the fixed self-authored argv0 pointer",
            "wrong argv0-pointer witness contract")

    for label, record in evidence["images"].items():
        path = ROOT / record["path"]
        require(path.is_file() and path.stat().st_size > 0, f"missing or empty {label}")
        require(digest(path) == record["sha256"], f"{label} SHA-256 mismatch")
    config = bound_text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_EXECVE_REPLY_ARGV0_POINTER_M0_PROBE" in config,
            "Phase 90 selector disabled")

    sources = {
        name: bound_text(record, name)
        for name, record in evidence["implementation"].items()
    }
    cmake = sources["build_gate"]
    for token in ("SeLinExecveReplyArgv0PointerM0", "DEFAULT\n    OFF",
                  "selinos-execve-reply-argv0-pointer-m0-fixture",
                  "execve_reply_argv0_pointer_m0_root.c"):
        require(token in cmake, f"missing Phase 90 build control: {token}")
    root_wiring = sources["root_wiring"]
    for token in ("CONFIG_SELINOS_EXECVE_REPLY_ARGV0_POINTER_M0_PROBE",
                  "selinos_execve_reply_argv0_pointer_m0_start",
                  "mov rax,[rsp+8]",
                  "no normal 18-word reply-frame"):
        require(token in root_wiring, f"missing Phase 90 root wiring: {token}")

    transaction = sources["root_transaction"]
    for token in ("seL4_Fault_UnknownSyscall", "seL4_TCB_ReadRegisters",
                  "seL4_TCB_WriteRegisters", "seL4_UnknownSyscall_FaultIP + 1u",
                  "seL4_Reply(", "seL4_Fault_UserException",
                  "POST_READ_UD2_OFFSET", "EXPECTED_ARGV0_POINTER",
                  "[2u] = 0x44u", "[3u] = 0x24u", "[4u] = 0x08u",
                  "[5u] = 0x0fu", "[6u] = 0x0bu"):
        require(token in transaction, f"missing Phase 90 transaction control: {token}")
    require("seL4_UnknownSyscall_SP," not in transaction,
            "bridge must not write SP through the reply frame")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "more than one target resume")

    protocol = sources["protocol"]
    for token in ("LINUX_EXECVE_SYSCALL 59u", "EXPECTED_ARGV0_POINTER 0x70002f00u",
                  "POST_READ_UD2_OFFSET 5u", "X86_SYSCALL_REPLY_REGISTERS 18u",
                  "FIXED_STACK_VADDR 0x70002000u", "STACK_ARGV0_WORD 1u"):
        require(token in protocol, f"missing Phase 90 protocol control: {token}")

    runtime = bound_text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("normal 18-word" in gate and "not claim" in gate,
            "gate fails to distinguish normal reply-frame non-claim")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("normal 18-word", "general `execve`", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")
    print("SeLinOS execve reply argv0-pointer M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
