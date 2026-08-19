#!/usr/bin/env python3
"""Independent verifier for Phase 78 x86 NXE fresh-stack revalidation M3."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "tests/artifacts/selinos_fresh_target_nxe_stack_revalidation_m3.verification.json"
SOURCE = ROOT / "src/projects/helixos/src/domain_manager.c"
CMAKE = ROOT / "src/projects/helixos/CMakeLists.txt"
HEAD = ROOT / "src/kernel/src/arch/x86/64/head.S"
PROTOCOL = ROOT / "src/projects/helixos/include/selinos_fresh_target_stack_vmfault_fsr_m2_protocol.h"


def fail(message: str) -> None:
    print(f"verification failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    evidence = json.loads(EVIDENCE.read_text())
    require(evidence["gate"] == "Phase 78 x86 NXE fresh-stack revalidation M3",
            "wrong evidence gate")
    require(evidence["observed_branch"] == "post_read_user_exception",
            "M3 must record only the terminal post-read UserException branch")
    require(evidence["status"].startswith("verified;"), "M3 evidence is not verified")

    checks: list[tuple[Path, str]] = []
    for item in evidence["images"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    checks.append((ROOT / evidence["runtime"]["path"], evidence["runtime"]["sha256"]))
    for item in evidence["implementation"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    for path, expected in checks:
        require(path.is_file(), f"missing SHA-bound file {path.relative_to(ROOT)}")
        require(sha256(path) == expected, f"SHA-256 mismatch for {path.relative_to(ROOT)}")

    cmake = CMAKE.read_text()
    option_begin = cmake.find("SeLinFreshTargetNxeStackRevalidationProbe")
    require(option_begin >= 0, "missing M3 CMake option")
    option = cmake[option_begin:cmake.find(")", option_begin) + 1]
    require("SELINOS_FRESH_TARGET_NXE_STACK_REVALIDATION_PROBE" in option,
            "missing M3 configuration macro")
    require("DEFAULT\n    OFF" in option, "M3 profile must default OFF")

    head = HEAD.read_text()
    for token in (
        "BEGIN_FUNC(nxe_check)",
        "cpuid",
        "nxe_error_string",
        "orl $0x900, %eax",
        "call nxe_check",
        "wrmsr",
    ):
        require(token in head, f"x86 head path missing {token}")
    require(head.find("call nxe_check") < head.find("orl $0x900, %eax"),
            "NX capability gate must precede LME+NXE programming")

    protocol = PROTOCOL.read_text()
    for token in (
        "FIXED_ENTRY_VADDR 0x60000000u",
        "FIXED_STACK_POINTER 0x70002ff8u",
        "POST_READ_UD2_VADDR 0x60000004u",
        "INVALID_OPCODE_VECTOR 6u",
        "FAULT_BADGE 0x74u",
    ):
        require(token in protocol, f"M3 shared protocol missing {token}")

    source = SOURCE.read_text()
    guard = ("CONFIG_SELINOS_FRESH_TARGET_STACK_VMFAULT_FSR_PROBE || \\\n"
             "    CONFIG_SELINOS_FRESH_TARGET_NXE_STACK_REVALIDATION_PROBE")
    require(source.count(guard) >= 3, "M3 must inherit the existing Phase 78 mapping transaction")
    require(source.count("seL4_TCB_Resume(fresh_tcb.cptr)") >= 1,
            "shared transaction lacks the fresh-target resume")
    require("seL4_Fault_UserException" in source, "M3 lacks terminal UserException classification")
    for token in (
        "POST_READ_UD2_VADDR",
        "FIXED_STACK_POINTER",
        "INVALID_OPCODE_VECTOR",
        "CPUID-gated NXE path reached exact post-read terminal invalid-opcode witness",
    ):
        require(token in source, f"M3 source missing {token}")

    log = (ROOT / evidence["runtime"]["path"]).read_text(errors="replace")
    for marker in evidence["runtime"]["required_markers"]:
        require(marker in log, f"missing QEMU marker: {marker}")
    for forbidden in evidence["runtime"]["forbidden_markers"]:
        require(forbidden not in log, f"forbidden QEMU marker present: {forbidden}")
    require("SeLinOS M0: isolated domain bootstrap FAILED." not in log,
            "QEMU reported bootstrap failure")
    require("raw FSR 0xc" not in log, "reserved-bit FSR branch remains present")

    print("verification passed: Phase 78 x86 NXE fresh-stack revalidation M3 post-read terminal witness")


if __name__ == "__main__":
    main()
