#!/usr/bin/env python3
"""Independently verify the bounded Phase 85 ET_DYN relative-relocation proof."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_controlled_et_dyn_relative_m0.verification.json"


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
            "SeLinOS controlled ELF64 ET_DYN relative-relocation M0",
            "wrong milestone")
    require(evidence.get("status") == "verified", "milestone is not verified")

    profile = evidence["profile"]
    require(profile["architecture"] == "x86_64/PC99", "wrong platform")
    require(profile["cmake_option"] == "SeLinControlledEtDynRelativeM0=ON",
            "wrong isolated selector")
    require(profile["relative_relocation"] ==
            "one R_X86_64_RELATIVE: *(load_base + 0x1008) = load_base + 0x3000",
            "wrong relocation contract")
    require(profile["entry_mapping"] == "read/execute, non-writable",
            "wrong entry mapping")
    require(profile["ro_mapping"] == "read-only plus execute-disable",
            "wrong read-only mapping")
    require(profile["data_mapping"] == "read/write plus execute-disable",
            "wrong data mapping")

    for label, record in evidence["images"].items():
        require(bind(record, f"{label} image").stat().st_size > 0,
                f"{label} image is empty")
    config = text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_CONTROLLED_ET_DYN_RELATIVE_M0_PROBE" in config and
            "/* disabled: CONFIG_SELINOS_CONTROLLED_ET_DYN_RELATIVE_M0_PROBE */" not in config,
            "Phase 85 selector not enabled")

    sources = {label: text(record, label)
               for label, record in evidence["implementation"].items()}
    parser = sources["elfrt_parser"]
    for required in ("ELFRT_DT_RELA", "ELFRT_DT_RELASZ", "ELFRT_DT_RELAENT",
                     "summary->rela_address", "summary->rela_size",
                     "summary->rela_entry_size"):
        require(required in parser, f"missing relocation metadata parser guard: {required}")

    cmake = sources["build_gate"]
    for required in ("SeLinControlledEtDynRelativeM0",
                     "SELINOS_CONTROLLED_ET_DYN_RELATIVE_M0_PROBE",
                     "selinos-controlled-et-dyn-relative-m0-fixture",
                     "controlled_et_dyn_relative_m0_root.c", "DEFAULT\n    OFF"):
        require(required in cmake, f"missing Phase 85 CMake guard: {required}")

    fixture = sources["fixture"]
    for required in ("[16] = 3u", "[344] = 2u", "[348] = 4u",
                     "DT_RELA=0x2100", "DT_RELASZ=24", "DT_RELAENT=24",
                     "Elf64_Rela: r_offset=0x1008", "RELA_TYPE 8u",
                     "summary->elf_type != 3u", "summary->has_dynamic != 1u",
                     "summary->has_rela != 1u", "summary->rela_address != 0x2100u",
                     "summary->rela_size != 24u", "summary->rela_entry_size != 24u",
                     "SECOND_RX_PAYLOAD_BYTES", "SUCCESS_UD2_OFFSET 89u"):
        require(required in fixture or required in sources["protocol"],
                f"missing fixed ET_DYN relocation guard: {required}")

    transaction = sources["root_transaction"]
    for required in ("summary.entry != 0u", "summary.rela_address != 0x2100u",
                     "summary.rela_size != 24u", "summary.rela_entry_size != 24u",
                     "FIXED_LOAD_BASE", "RELA_ADDEND", "RELA_DESTINATION_OFFSET",
                     "FIXED_SECOND_RX_VADDR", "seL4_X86_ExecuteDisable",
                     "seL4_TCB_Resume(target_tcb.cptr)", "seL4_Fault_UserException"):
        require(required in transaction, f"missing relocation transaction guard: {required}")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "transaction contains more than one target resume")
    require("seL4_Reply(" not in transaction and "seL4_Send(" not in transaction,
            "transaction contains reply/send continuation path")
    data_unmap = transaction.index("vspace_unmap_pages(vspace, root_data_mapping")
    target_data_map = transaction.index(
        "(void *)SELINOS_CONTROLLED_ET_DYN_RELATIVE_M0_FIXED_DATA_VADDR")
    require(data_unmap < target_data_map,
            "root writable relocation alias survives target data mapping")

    runtime = text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("Status:** VERIFIED" in gate, "phase gate is not marked verified")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("general ELF loader", "dynamic linker", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")

    print("SeLinOS controlled ELF64 ET_DYN relative-relocation M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"verification failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
