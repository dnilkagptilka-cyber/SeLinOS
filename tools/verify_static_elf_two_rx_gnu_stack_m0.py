#!/usr/bin/env python3
"""Independently verify the bounded Phase 83 two-RX ELF64 M0 proof."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_static_elf_two_rx_gnu_stack_m0.verification.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


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
    require(evidence.get("milestone") ==
            "SeLinOS static ELF64 two-RX plus PT_GNU_STACK load M0",
            "wrong milestone")
    require(evidence.get("status") == "verified", "milestone is not verified")

    profile = evidence["profile"]
    require(profile["cmake_option"] == "SeLinStaticElfTwoRxGnuStackM0=ON",
            "wrong isolated profile selector")
    require(profile["architecture"] == "x86_64/PC99", "wrong platform")
    require(profile["entry_mapping"] == "read/execute, non-writable",
            "wrong entry target policy")
    require(profile["second_rx_mapping"] == "read/execute, non-writable",
            "wrong second RX target policy")
    require(profile["ro_mapping"] == "read-only plus execute-disable",
            "wrong read-only target policy")
    require(profile["data_mapping"] == "read/write plus execute-disable",
            "wrong data target policy")
    require(profile["bss"] == "p_filesz=4, p_memsz=64, exactly 60 zero-filled bytes",
            "wrong bounded BSS contract")

    for label, record in evidence["images"].items():
        image = bind(record, f"{label} image")
        require(image.stat().st_size > 0, f"{label} image is empty")
    config = text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_PROBE" in config and
            "/* disabled: CONFIG_SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_PROBE */" not in config,
            "Phase 83 selector not enabled in isolated generated config")

    sources = {label: text(record, label)
               for label, record in evidence["implementation"].items()}
    cmake = sources["build_gate"]
    for required in (
        "SeLinStaticElfTwoRxGnuStackM0",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_PROBE",
        "DEFAULT\n    OFF",
        "selinos-static-elf-two-rx-gnu-stack-m0-fixture",
        "static_elf_two_rx_gnu_stack_m0_root.c",
    ):
        require(required in cmake, f"missing Phase 83 CMake contract: {required}")

    fixture = sources["fixture"]
    for required in (
        "[56] = 5u",
        "[64] = 1u",
        "[68] = 5u",
        "[120] = 1u",
        "[124] = 4u",
        "[176] = 1u",
        "[180] = 6u",
        "[232] = 1u",
        "[236] = 5u",
        "[288] = 0x51u, [289] = 0xe5u, [290] = 0x74u, [291] = 0x64u",
        "summary->has_gnu_stack != 1u",
        "summary->gnu_stack_flags != 6u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_ENTRY_PAYLOAD_BYTES",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_SECOND_RX_PAYLOAD_BYTES",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_RO_INITIALIZED_BYTES",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_DATA_MEMORY_BYTES",
        "summary->load_segments != 4u",
        "summary->writable_load_segments != 1u",
        "summary->executable_load_segments != 2u",
        "summary->writable_executable_load_segments != 0u",
        "selinos_elfrt_parse_image(image, image_bytes, 0, summary)",
        "selinos_elfrt_validate_initial_load_policy(summary)",
        "static_elf_two_rx_ro_rw_bss_fixture",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_BSS_WRITE_VALUE",
        "[SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_ENTRY_LOAD_OFFSET + 0u] = 0xe9u",
    ):
        require(required in fixture,
                f"missing fixed four-segment two-RX fixture/parser guard: {required}")

    transaction = sources["root_transaction"]
    for required in (
        "const seL4_CapRights_t read_only = seL4_CapRights_new(0, 0, 1, 0)",
        "const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1)",
        "target_second_rx_frame",
        "root_second_rx_mapping",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_ENTRY_VADDR",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_SECOND_RX_VADDR",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_RO_VADDR",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_DATA_VADDR",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_BSS_OFFSET",
        "seL4_X86_ExecuteDisable",
        "seL4_TCB_Resume(target_tcb.cptr)",
        "seL4_Fault_UserException",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_SUCCESS_UD2_OFFSET",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_INVALID_OPCODE_VECTOR",
    ):
        require(required in transaction, f"missing transaction guard: {required}")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "transaction contains more than one target resume")
    require("seL4_Reply(" not in transaction and "seL4_Send(" not in transaction,
            "transaction contains reply/send continuation path")
    entry_unmap = transaction.index("vspace_unmap_pages(vspace, root_entry_mapping")
    second_unmap = transaction.index("vspace_unmap_pages(vspace, root_second_rx_mapping")
    ro_unmap = transaction.index("vspace_unmap_pages(vspace, root_ro_mapping")
    data_unmap = transaction.index("vspace_unmap_pages(vspace, root_data_mapping")
    target_entry_map = transaction.index(
        "(void *)SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_ENTRY_VADDR")
    target_second_map = transaction.index(
        "(void *)SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_SECOND_RX_VADDR")
    target_ro_map = transaction.index(
        "(void *)SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_RO_VADDR")
    target_data_map = transaction.index(
        "(void *)SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_DATA_VADDR")
    require(entry_unmap < target_entry_map and second_unmap < target_second_map and
            ro_unmap < target_ro_map and data_unmap < target_data_map,
            "root writable aliases are not unmapped before target mappings")
    fault_check = transaction.index("seL4_GetMR(seL4_UserException_FaultIP)")
    require(transaction.index(
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_SECOND_RX_VADDR", fault_check) >
            fault_check,
            "terminal fault is not bound to the second RX page")

    protocol = sources["protocol"]
    for required in (
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FAULT_BADGE 0x83u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_ENTRY_VADDR 0x60000000u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_DATA_VADDR 0x60001000u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_RO_VADDR 0x60002000u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_FIXED_SECOND_RX_VADDR 0x60003000u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_ENTRY_PAYLOAD_BYTES 5u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_SECOND_RX_PAYLOAD_BYTES 72u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_RO_INITIALIZED_BYTES 4u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_DATA_INITIALIZED_BYTES 4u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_DATA_MEMORY_BYTES 64u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_BSS_BYTES 60u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_DATA_INITIAL_VALUE 0x534c3832u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_RO_INITIAL_VALUE 0x524f3832u",
        "SELINOS_STATIC_ELF_TWO_RX_GNU_STACK_M0_SUCCESS_UD2_OFFSET 70u",
    ):
        require(required in protocol, f"missing protocol constant: {required}")

    runtime = text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("Status:** VERIFIED" in gate, "phase gate is not marked verified")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("arbitrary `PT_LOAD`", "dynamic linker", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")

    print("SeLinOS static ELF64 two-RX plus PT_GNU_STACK load M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"verification failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
