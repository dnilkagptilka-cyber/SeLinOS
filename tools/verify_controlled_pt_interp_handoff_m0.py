#!/usr/bin/env python3
"""Independently verify the bounded Phase 86 PT_INTERP handoff proof."""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/artifacts/selinos_controlled_pt_interp_handoff_m0.verification.json"


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
    require(evidence.get("schema") == 1, "invalid evidence schema")
    require(evidence.get("milestone") == "SeLinOS controlled PT_INTERP handoff M0",
            "wrong milestone")
    require(evidence.get("status") == "verified", "milestone is not verified")
    profile = evidence["profile"]
    require(profile["architecture"] == "x86_64/PC99", "wrong architecture")
    require(profile["cmake_option"] == "SeLinControlledPtInterpHandoffM0=ON",
            "wrong isolated option")
    require(profile["handoff"] == "one self-authored interpreter RX entry with bounded argc/auxv witness",
            "wrong handoff contract")

    for label, record in evidence["images"].items():
        require(bind(record, f"{label} image").stat().st_size > 0, f"empty {label} image")
    config = text(evidence["generated_config"], "generated config")
    require("#define CONFIG_SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_PROBE" in config and
            "/* disabled: CONFIG_SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_PROBE */" not in config,
            "Phase 86 selector is not enabled")

    sources = {name: text(record, name) for name, record in evidence["implementation"].items()}
    cmake = sources["build_gate"]
    for required in ("SeLinControlledPtInterpHandoffM0",
                     "SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_PROBE",
                     "selinos-controlled-pt-interp-handoff-m0-fixture",
                     "controlled_pt_interp_handoff_m0_root.c", "DEFAULT\n    OFF"):
        require(required in cmake, f"missing build guard: {required}")

    fixture = sources["fixture"]
    for required in ("[16] = 3u", "[56] = 7u", "[400] = 3u", "[404] = 4u",
                     "Exact non-host interpreter witness pathname",
                     "selinos_elfrt_parse_image(image, image_bytes, 1, summary)",
                     "summary->has_interp != 1u", "summary->interpreter[0] != '/'",
                     "summary->interpreter[23] != '\\0'", "summary->has_dynamic != 1u",
                     "summary->has_rela != 1u"):
        require(required in fixture, f"missing PT_INTERP fixture guard: {required}")

    protocol = sources["protocol"]
    for required in ("FAULT_BADGE 0x86u", "FIXED_INTERPRETER_VADDR 0x60004000u",
                     "FIXED_STACK_POINTER 0x70002f80u", "STACK_ALIGNMENT 16u",
                     "AUXV_AT_PHDR", "AUXV_AT_PHENT", "AUXV_AT_PHNUM",
                     "AUXV_AT_PAGESZ", "AUXV_AT_ENTRY", "STACK_AUXV_PAIRS 6u"):
        require(required in protocol, f"missing protocol control: {required}")

    transaction = sources["root_transaction"]
    for required in ("target_interpreter_frame", "root_interpreter_mapping",
                     "root_stack_mapping", "FIXED_INTERPRETER_VADDR",
                     "FIXED_STACK_POINTER", "AUXV_AT_ENTRY", "AUXV_AT_PHDR",
                     "seL4_X86_ExecuteDisable", "seL4_TCB_Resume(target_tcb.cptr)",
                     "seL4_Fault_UserException"):
        require(required in transaction, f"missing handoff transaction control: {required}")
    require(transaction.count("seL4_TCB_Resume(target_tcb.cptr)") == 1,
            "more than one target resume")
    require("seL4_Reply(" not in transaction and "seL4_Send(" not in transaction,
            "reply/send continuation present")
    require(transaction.index("vspace_unmap_pages(vspace, root_stack_mapping") <
            transaction.index("(void *)SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_FIXED_STACK_VADDR"),
            "root stack alias survives target stack mapping")
    require(transaction.index("vspace_unmap_pages(vspace, root_interpreter_mapping") <
            transaction.index("(void *)SELINOS_CONTROLLED_PT_INTERP_HANDOFF_M0_FIXED_INTERPRETER_VADDR"),
            "root interpreter alias survives target interpreter mapping")

    runtime = text(evidence["runtime_evidence"], "runtime transcript")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden runtime marker: {marker}")

    gate = sources["phase_gate"]
    require("Status:** VERIFIED" in gate, "gate not marked verified")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("general ELF loader", "dynamic linker", "Linux ABI", "`dpkg`", "`apt`"):
        require(phrase in exclusions, f"missing non-claim: {phrase}")
    print("SeLinOS controlled PT_INTERP handoff M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
