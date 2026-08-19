#!/usr/bin/env python3
"""Independent verifier for Phase 78 fresh-target stack data-access M1."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "tests/artifacts/selinos_fresh_target_stack_data_access_m1.verification.json"
SOURCE = ROOT / "src/projects/helixos/src/domain_manager.c"
CMAKE = ROOT / "src/projects/helixos/CMakeLists.txt"
PROTOCOL = ROOT / "src/projects/helixos/include/selinos_fresh_target_stack_data_access_m1_protocol.h"


def fail(message: str) -> None:
    print(f"verification failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def direct_blocks(text: str, macro: str) -> list[str]:
    marker = f"#if {macro}"
    blocks: list[str] = []
    offset = 0
    while True:
        begin = text.find(marker, offset)
        if begin < 0:
            return blocks
        depth = 0
        cursor = begin
        while cursor < len(text):
            line_end = text.find("\n", cursor)
            if line_end < 0:
                line_end = len(text)
            line = text[cursor:line_end]
            if line.startswith("#if"):
                depth += 1
            elif line.startswith("#endif"):
                depth -= 1
                if depth == 0:
                    blocks.append(text[begin:line_end + len("#endif")])
                    offset = line_end + len("#endif")
                    break
            cursor = line_end + 1
        else:
            fail(f"unterminated guard {macro}")


def main() -> None:
    evidence = json.loads(EVIDENCE.read_text())
    require(evidence["gate"] == "Phase 78 fresh target stack data-access M1", "wrong evidence gate")
    branch = evidence["observed_branch"]
    require(branch in ("blocked_data_vmfault", "post_read_user_exception"),
            "manifest must record a supported M1 classification branch")
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
    option_begin = cmake.find("SeLinFreshTargetStackDataAccessProbe")
    require(option_begin >= 0, "missing M1 CMake option")
    option = cmake[option_begin:cmake.find(")", option_begin) + 1]
    require("SELINOS_FRESH_TARGET_STACK_DATA_ACCESS_PROBE" in option, "missing M1 config macro")
    require("DEFAULT\n    OFF" in option, "M1 profile must default OFF")

    protocol = PROTOCOL.read_text()
    for token in (
        "FIXED_ENTRY_VADDR 0x60000000u",
        "FIXED_STACK_VADDR 0x70002000u",
        "FIXED_STACK_POINTER 0x70002ff8u",
        "POST_READ_UD2_VADDR 0x60000004u",
        "FAULT_BADGE 0x74u",
        "INVALID_OPCODE_VECTOR 6u",
        "PAYLOAD_BYTES 6u",
    ):
        require(token in protocol, f"protocol missing {token}")

    source = SOURCE.read_text()
    require("#if CONFIG_SELINOS_FRESH_TARGET_MAPPING_PROVENANCE_PROBE || \\\n    CONFIG_SELINOS_FRESH_TARGET_STACK_DATA_ACCESS_PROBE" in source,
            "M1 must inherit Phase 78 M0 mapping ledger")
    blocks = direct_blocks(source, "CONFIG_SELINOS_FRESH_TARGET_STACK_DATA_ACCESS_PROBE")
    require(len(blocks) >= 3, "expected M1 payload, classification and diagnostic guards")
    payload, classification, diagnostic = blocks[0], blocks[1], blocks[-1]

    expected_bytes = ["0x48u", "0x8bu", "0x04u", "0x24u", "0x0fu", "0x0bu"]
    positions = [payload.find(byte) for byte in expected_bytes]
    require(all(position >= 0 for position in positions), "M1 payload missing exact byte")
    require(positions == sorted(positions), "M1 payload byte order changed")
    for required in (
        "vspace_map_pages(vspace, &fresh_entry_frame.cptr",
        "vspace_unmap_pages(vspace, fresh_entry_root_mapping",
        "VSPACE_PRESERVE",
        "SELINOS_FRESH_TARGET_STACK_DATA_ACCESS_M1_FIXED_ENTRY_VADDR",
        "SELINOS_FRESH_TARGET_STACK_DATA_ACCESS_M1_FIXED_STACK_POINTER",
    ):
        require(required in payload, f"M1 payload block missing {required}")
    for forbidden in ("fresh_stack_frame", "seL4_TCB_Resume", "seL4_Recv", "seL4_Reply", "seL4_SetMR"):
        require(forbidden not in payload, f"payload block must not contain {forbidden}")

    require(classification.count("seL4_TCB_Resume(fresh_tcb.cptr)") == 1, "M1 requires exactly one fresh resume")
    require(classification.count("seL4_Recv(fresh_fault_endpoint.cptr, &fault_badge)") == 1, "M1 requires exactly one fault receive")
    for required in (
        "seL4_Fault_UserException",
        "seL4_Fault_VMFault",
        "seL4_VMFault_IP",
        "seL4_VMFault_Addr",
        "seL4_VMFault_PrefetchFault",
        "POST_READ_UD2_VADDR",
        "FIXED_STACK_POINTER",
        "FIXED_ENTRY_VADDR",
        "initial rsp data read produced classified VMFault",
    ):
        require(required in classification, f"classification block missing {required}")
    for forbidden in ("seL4_Reply", "seL4_SetMR", "fresh_entry_root_mapping", "vspace_map_pages", "vspace_unmap_pages"):
        require(forbidden not in classification, f"classification block must not contain {forbidden}")
    require("one classified initial-rsp read only" in diagnostic, "missing M1 diagnostic boundary")

    log = (ROOT / evidence["runtime"]["path"]).read_text(errors="replace")
    for marker in evidence["runtime"]["required_markers"]:
        require(marker in log, f"missing QEMU marker: {marker}")
    blocked_marker = (
        "SeLinOS fresh target stack data-access M1: initial rsp data read produced "
        "classified VMFault; no reply, repair or second resume."
    )
    post_read_marker = (
        "SeLinOS fresh target stack data-access M1: one mov rax,[rsp] completed then "
        "terminal invalid-opcode fault received; no reply or second resume."
    )
    if branch == "blocked_data_vmfault":
        require(blocked_marker in log, "manifest names blocked branch but blocked marker is absent")
        require(post_read_marker not in log,
                "manifest names blocked branch but post-read terminal marker is present")
    else:
        require(post_read_marker in log, "post-NXE M1 branch lacks terminal post-read marker")
        require(blocked_marker not in log,
                "post-NXE M1 branch unexpectedly reports the historical blocked marker")
    require("SeLinOS M0: isolated domain bootstrap FAILED." not in log, "QEMU reported bootstrap failure")

    print(f"verification passed: Phase 78 fresh target stack data-access M1 {branch}")


if __name__ == "__main__":
    main()
