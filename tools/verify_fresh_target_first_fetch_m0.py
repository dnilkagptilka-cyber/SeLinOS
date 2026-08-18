#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def config_block_before(source, anchor):
    anchor_offset = source.index(anchor)
    start = source.rfind("#if ", 0, anchor_offset)
    depth = 0
    cursor = start
    while cursor < len(source):
        next_line = source.find("\n", cursor)
        if next_line < 0:
            next_line = len(source)
        line = source[cursor:next_line].lstrip()
        if line.startswith("#if"):
            depth += 1
        elif line.startswith("#endif"):
            depth -= 1
            if depth == 0:
                return source[start:next_line]
        cursor = next_line + 1
    raise RuntimeError("unterminated preprocessor block")


try:
    root = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (root / "tests/artifacts/selinos_fresh_target_first_fetch_m0.verification.json").read_text()
    )

    for group in ("images", "implementation"):
        for item in evidence[group].values():
            path = root / item["path"]
            require(path.is_file(), "missing " + item["path"])
            require(sha256(path) == item["sha256"], "SHA mismatch: " + item["path"])

    runtime = root / evidence["runtime"]["path"]
    require(runtime.is_file(), "missing runtime log")
    require(sha256(runtime) == evidence["runtime"]["sha256"], "runtime SHA mismatch")
    runtime_text = runtime.read_text(errors="replace")
    for marker in evidence["runtime"]["required_markers"]:
        require(marker in runtime_text, "missing runtime marker: " + marker)

    source = (root / evidence["implementation"]["root"]["path"]).read_text()
    cmake = (root / evidence["implementation"]["cmake"]["path"]).read_text()
    protocol = (root / evidence["implementation"]["protocol"]["path"]).read_text()
    require("SeLinFreshTargetFirstFetchProbe" in cmake, "default-OFF Phase 76 profile absent")
    for constant in (
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_ENTRY_VADDR 0x60000000u",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_STACK_VADDR 0x70002000u",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_POST_NOP_VADDR 0x60000001u",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_STACK_POINTER 0x70002ff8u",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_INVALID_OPCODE_VECTOR 6u",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FAULT_BADGE 0x74u",
    ):
        require(constant in protocol, "protocol constant absent: " + constant)

    phase75 = config_block_before(
        source, "requested_context.rip = SELINOS_FRESH_TARGET_CONTEXT_PROVENANCE_M0_FIXED_RIP"
    )
    for required in (
        "CONFIG_SELINOS_FRESH_TARGET_FIRST_FETCH_PROBE",
        "seL4_TCB_WriteRegisters(fresh_tcb.cptr",
        "seL4_TCB_ReadRegisters(fresh_tcb.cptr",
        "observed_context.rip != requested_context.rip",
        "observed_context.rsp != requested_context.rsp",
    ):
        require(required in phase75, "missing Phase 75 prerequisite: " + required)

    phase76 = config_block_before(
        source, "fresh_entry_root_mapping = vspace_map_pages(vspace, &fresh_entry_frame.cptr"
    )
    for required in (
        "vspace_map_pages(vspace, &fresh_entry_frame.cptr",
        "((volatile uint8_t *)fresh_entry_root_mapping)[0] = 0x90u",
        "((volatile uint8_t *)fresh_entry_root_mapping)[1] = 0x0fu",
        "((volatile uint8_t *)fresh_entry_root_mapping)[2] = 0x0bu",
        "vspace_unmap_pages(vspace, fresh_entry_root_mapping",
        "fresh_entry_frame.cptr",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_ENTRY_VADDR",
        "seL4_X86_Default_VMAttributes",
        "fresh_stack_frame.cptr",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_STACK_VADDR",
        "seL4_X86_ExecuteDisable",
        "fault_message = seL4_Recv(fresh_fault_endpoint.cptr, &fault_badge)",
        "seL4_Fault_UserException",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_POST_NOP_VADDR",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_STACK_POINTER",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_INVALID_OPCODE_VECTOR",
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FAULT_BADGE",
    ):
        require(required in phase76, "missing Phase 76 control: " + required)
    require(phase76.count("seL4_TCB_Resume(fresh_tcb.cptr)") == 1, "Phase 76 must resume once")
    for forbidden in ("seL4_Reply(", "seL4_Send(", "seL4_Call(", "seL4_TCB_Suspend("):
        require(forbidden not in phase76, "forbidden Phase 76 continuation control: " + forbidden)

    print("SeLinOS fresh target first-fetch M0 evidence verified.")
except Exception as error:
    print("verification failed: " + str(error), file=sys.stderr)
    sys.exit(1)
