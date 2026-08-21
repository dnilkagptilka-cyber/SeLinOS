#!/usr/bin/env python3
"""Independently verify the bounded Phase 87 execve replacement M0 proof."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_controlled_execve_replacement_m0.verification.json"


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bind(record: dict, label: str) -> Path:
    path = ROOT / record["path"]
    require(path.is_file(), f"missing {label}")
    require(digest(path) == record["sha256"], f"{label} SHA-256 mismatch")
    return path


def text(record: dict, label: str) -> str:
    return bind(record, label).read_text(errors="replace")


def main() -> int:
    evidence = json.loads(MANIFEST.read_text())
    require(evidence.get("schema") == 1, "invalid schema")
    require(evidence.get("milestone") == "SeLinOS controlled execve replacement M0",
            "wrong milestone")
    require(evidence.get("status") == "verified", "milestone is not verified")
    profile = evidence["profile"]
    require(profile["architecture"] == "x86_64/PC99", "wrong architecture")
    require(profile["cmake_option"] == "SeLinControlledExecveReplacementM0=ON",
            "wrong isolated option")
    require(profile["transition"] == "one fixed Linux execve(59) fault redirected to stack-independent RX interpreter UD2",
            "wrong replacement contract")

    for label, record in evidence["images"].items():
        require(bind(record, f"{label} image").stat().st_size > 0, f"empty {label} image")
    config = text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_CONTROLLED_EXECVE_REPLACEMENT_M0_PROBE" in config and
            "/* disabled: CONFIG_SELINOS_CONTROLLED_EXECVE_REPLACEMENT_M0_PROBE */" not in config,
            "Phase 87 selector disabled")
    sources = {name: text(record, name) for name, record in evidence["implementation"].items()}

    cmake = sources["build_gate"]
    for required in ("SeLinControlledExecveReplacementM0",
                     "SELINOS_CONTROLLED_EXECVE_REPLACEMENT_M0_PROBE",
                     "selinos-controlled-execve-replacement-m0-fixture",
                     "controlled_execve_replacement_m0_root.c", "DEFAULT\n    OFF"):
        require(required in cmake, f"missing build guard: {required}")

    fixture = sources["fixture"]
    for required in ("Linux execve(59); syscall", "ENTRY_PAYLOAD_BYTES", "!= 0xb8u",
                     "[400] = 3u", "summary->has_interp != 1u", "summary->has_rela != 1u"):
        require(required in fixture, f"missing fixture guard: {required}")
    protocol = sources["protocol"]
    for required in ("FAULT_BADGE 0x87u", "LINUX_EXECVE_SYSCALL 59u",
                     "X86_SYSCALL_REPLY_REGISTERS 18u", "FIXED_INTERPRETER_VADDR 0x60004000u",
                     "REPLACEMENT_GENERATION 1u"):
        require(required in protocol, f"missing protocol guard: {required}")

    transaction = sources["root_transaction"]
    for required in ("seL4_Fault_UnknownSyscall", "seL4_UnknownSyscall_RAX",
                     "LINUX_EXECVE_SYSCALL", "seL4_UnknownSyscall_FaultIP",
                     "FIXED_INTERPRETER_VADDR", "seL4_Reply(",
                     "seL4_Fault_UserException", "seL4_X86_ExecuteDisable"):
        require(required in transaction, f"missing replacement control: {required}")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "more than one target resume")
    require(transaction.index("vspace_unmap_pages(vspace, root_interpreter_mapping") <
            transaction.index("(void *)SELINOS_CONTROLLED_EXECVE_REPLACEMENT_M0_FIXED_INTERPRETER_VADDR"),
            "root interpreter alias survives target mapping")

    runtime = text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("Recorded blocker" in gate and "initial stack/auxv" in gate and
            "preservation or restoration" in gate,
            "gate fails to record the stack handoff blocker")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("general `execve`", "fork", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")
    print("SeLinOS controlled execve replacement M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
