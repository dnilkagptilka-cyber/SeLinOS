#!/usr/bin/env python3
"""Independent verifier for Phase 78 fresh-target mapping provenance M0."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "tests/artifacts/selinos_fresh_target_mapping_provenance_m0.verification.json"
SOURCE = ROOT / "src/projects/helixos/src/domain_manager.c"
CMAKE = ROOT / "src/projects/helixos/CMakeLists.txt"
PROTOCOL = ROOT / "src/projects/helixos/include/selinos_fresh_target_mapping_provenance_m0_protocol.h"


def fail(message: str) -> None:
    print(f"verification failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def guarded_block(text: str, macro: str, start: int = 0) -> str:
    marker = f"#if {macro}"
    begin = text.find(marker, start)
    require(begin >= 0, f"missing guard {macro}")
    depth = 0
    for line_end in range(begin, len(text)):
        if text.startswith("#if", line_end) and (line_end == 0 or text[line_end - 1] == "\n"):
            depth += 1
        elif text.startswith("#endif", line_end) and (line_end == 0 or text[line_end - 1] == "\n"):
            depth -= 1
            if depth == 0:
                return text[begin:line_end + len("#endif")]
    fail(f"unterminated guard {macro}")
    return ""


def main() -> None:
    require(EVIDENCE.is_file(), "missing evidence manifest")
    evidence = json.loads(EVIDENCE.read_text())
    require(evidence["gate"] == "Phase 78 fresh target mapping provenance M0", "wrong evidence gate")
    require(evidence["status"].startswith(("candidate;", "verified;")), "invalid evidence status")

    checks = []
    for item in evidence["images"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    checks.append((ROOT / evidence["runtime"]["path"], evidence["runtime"]["sha256"]))
    for item in evidence["implementation"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    for path, expected in checks:
        require(path.is_file(), f"missing SHA-bound file {path.relative_to(ROOT)}")
        require(sha256(path) == expected, f"SHA-256 mismatch for {path.relative_to(ROOT)}")

    cmake = CMAKE.read_text()
    require("SeLinFreshTargetMappingProvenanceProbe" in cmake, "missing Phase 78 CMake option")
    require("SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE" in cmake, "missing Phase 78 config macro")
    cmake_option_start = cmake.index("SeLinFreshTargetMappingProvenanceProbe")
    cmake_option = cmake[cmake_option_start:cmake.find(")", cmake_option_start) + 1]
    require("DEFAULT\n    OFF" in cmake_option, "Phase 78 profile must default OFF")

    protocol = PROTOCOL.read_text()
    for required in (
        "FIXED_ENTRY_VADDR 0x60000000u",
        "FIXED_STACK_VADDR 0x70002000u",
        "FIXED_IPC_VADDR 0x70000000u",
        "EXPECTED_STACK_PAGING_OBJECTS 0u",
        "EXPECTED_ENTRY_PAGING_OBJECTS 1u",
    ):
        require(required in protocol, f"protocol missing {required}")

    source = SOURCE.read_text()
    for prerequisite in (
        "CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_CONTEXT_PROVENANCE_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FIRST_FETCH_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE",
        "CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_CONTEXT_PROVENANCE_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FIRST_FETCH_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE",
        "CONFIG_SELINOS_FRESH_TARGET_CONTEXT_PROVENANCE_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FIRST_FETCH_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE",
    ):
        require(prerequisite in source, "Phase 78 must inherit Phase 73–75 prerequisites")

    block = guarded_block(source, "CONFIG_SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE")
    require(block.find("fresh_stack_frame.cptr") < block.find("fresh_entry_frame.cptr"), "stack map must precede entry map")
    for required in (
        "SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_M0_FIXED_STACK_VADDR",
        "SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_M0_FIXED_ENTRY_VADDR",
        "seL4_X86_ExecuteDisable",
        "fresh_stack_paging_object_count !=\n            SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_M0_EXPECTED_STACK_PAGING_OBJECTS",
        "fresh_entry_paging_object_count !=\n            SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_M0_EXPECTED_ENTRY_PAGING_OBJECTS",
        "suspended NX stack map reused IPC hierarchy and RX entry map allocated one paging object",
    ):
        require(required in block, f"Phase 78 transaction missing {required}")
    for forbidden in (
        "seL4_TCB_Resume",
        "seL4_Recv",
        "seL4_Reply",
        "seL4_SetMR",
        "fresh_entry_root_mapping",
        "vspace_map_pages",
        "seL4_TCB_WriteRegisters",
        "seL4_TCB_ReadRegisters",
        "0x0fu",
        "0x0bu",
        "0xc3u",
    ):
        require(forbidden not in block, f"Phase 78 transaction must not contain {forbidden}")

    log = (ROOT / evidence["runtime"]["path"]).read_text(errors="replace")
    for marker in evidence["runtime"]["required_markers"]:
        require(marker in log, f"missing QEMU marker: {marker}")
    require("SeLinOS M0: isolated domain bootstrap FAILED." not in log, "QEMU reported bootstrap failure")

    print("verification passed: Phase 78 fresh target mapping provenance M0")


if __name__ == "__main__":
    main()
