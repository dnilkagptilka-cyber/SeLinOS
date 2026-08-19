#!/usr/bin/env python3
"""Independent verifier for the bounded Phase 79 static ELF64 RX-load M0 proof."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_static_elf_rx_load_m0.verification.json"


def fail(message: str) -> None:
    raise RuntimeError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bind(record: dict, label: str) -> Path:
    require(isinstance(record, dict), f"{label}: binding is not an object")
    require(isinstance(record.get("path"), str), f"{label}: missing path")
    require(isinstance(record.get("sha256"), str), f"{label}: missing sha256")
    path = ROOT / record["path"]
    require(path.is_file(), f"{label}: missing {path}")
    require(sha256(path) == record["sha256"], f"{label}: SHA-256 mismatch")
    return path


def text(record: dict, label: str) -> str:
    return bind(record, label).read_text(errors="replace")


def main() -> int:
    evidence = json.loads(MANIFEST.read_text())
    require(evidence.get("schema") == 1, "unsupported manifest schema")
    require(evidence.get("milestone") == "SeLinOS static ELF64 RX load M0",
            "wrong milestone")
    profile = evidence["profile"]
    require(profile["cmake_option"] == "SeLinStaticElfRxLoadM0=ON",
            "wrong default-OFF profile selector")
    require(profile["architecture"] == "x86_64/PC99", "wrong platform")
    require(profile["target_mapping"] == "read/execute, non-writable", "wrong target policy")

    for label, record in evidence["images"].items():
        image = bind(record, f"{label} image")
        require(image.stat().st_size > 0, f"{label} image is empty")
    config = text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_STATIC_ELF_RX_LOAD_M0_PROBE" in config and
            "/* disabled: CONFIG_SELINOS_STATIC_ELF_RX_LOAD_M0_PROBE */" not in config,
            "M0 selector not enabled in isolated generated config")

    sources = {label: text(record, label) for label, record in evidence["implementation"].items()}
    cmake = sources["build_gate"]
    for required in (
        "SeLinStaticElfRxLoadM0",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_PROBE",
        "DEFAULT\n    OFF",
        "selinos-static-elf-rx-load-m0-fixture",
        "static_elf_rx_load_m0_root.c",
    ):
        require(required in cmake, f"missing Phase 79 CMake contract: {required}")

    fixture = sources["fixture"]
    for required in (
        "static const selinos_elfrt_u8 static_elf_rx_fixture",
        "[64] = 1u",
        "[68] = 5u",
        "[73] = 0x10u",
        "[83] = 0x60u",
        "[96] = SELINOS_STATIC_ELF_RX_LOAD_M0_PAYLOAD_BYTES",
        "[104] = SELINOS_STATIC_ELF_RX_LOAD_M0_PAYLOAD_BYTES",
        "[113] = 0x10u",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE0",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_UD2_BYTE1",
        "selinos_elfrt_parse_image(image, image_bytes, 0, summary)",
        "selinos_elfrt_validate_initial_load_policy(summary)",
        "summary->has_dynamic != 0u",
        "summary->has_interp != 0u",
        "summary->has_rela != 0u",
        "summary->has_textrel != 0u",
    ):
        require(required in fixture, f"missing exact fixture/parser guard: {required}")

    transaction = sources["root_transaction"]
    for required in (
        "const seL4_CapRights_t read_only = seL4_CapRights_new(0, 0, 1, 0)",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_FIXED_ENTRY_VADDR",
        "seL4_X86_ExecuteDisable",
        "seL4_TCB_Resume(target_tcb.cptr)",
        "seL4_Fault_UserException",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_INVALID_OPCODE_VECTOR",
    ):
        require(required in transaction, f"missing transaction guard: {required}")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "transaction contains more than one target resume")
    require("seL4_Reply(" not in transaction and "seL4_Send(" not in transaction,
            "transaction contains reply/send continuation path")
    alias_unmap = transaction.index("vspace_unmap_pages(vspace, root_entry_mapping")
    target_rx_map = transaction.index("(void *)SELINOS_STATIC_ELF_RX_LOAD_M0_FIXED_ENTRY_VADDR")
    require(alias_unmap < target_rx_map,
            "root writable alias is not unmapped before target RX map")

    protocol = sources["protocol"]
    for required in (
        "SELINOS_STATIC_ELF_RX_LOAD_M0_FAULT_BADGE 0x79u",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_FIXED_ENTRY_VADDR 0x60000000u",
        "SELINOS_STATIC_ELF_RX_LOAD_M0_PAYLOAD_BYTES 2u",
    ):
        require(required in protocol, f"missing protocol constant: {required}")

    runtime = text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("Status: VERIFIED" in gate, "phase gate not marked verified")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("multiple `PT_LOAD`", "dynamic linking", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")

    print("SeLinOS static ELF64 RX load M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"verification failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
