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
    offset = source.index(anchor)
    start = source.rfind("#if ", 0, offset)
    end = source.index("\n#endif", start)
    return source[start:end]


try:
    root = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (root / "tests/artifacts/selinos_fresh_target_fault_reply_restart_m0.verification.json").read_text()
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
    require("SeLinFreshTargetFaultReplyRestartProbe" in cmake, "Phase 77 profile absent")
    for constant in (
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_FIRST_FAULT_VADDR 0x60000001u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_RESTART_VADDR 0x60000003u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_SECOND_FAULT_VADDR 0x60000004u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_STACK_POINTER 0x70002ff8u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_INVALID_OPCODE_VECTOR 6u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FAULT_BADGE 0x74u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_REPLY_WORDS 1u",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_PAYLOAD_BYTES 6u",
    ):
        require(constant in protocol, "protocol constant absent: " + constant)

    payload = config_block_before(
        source, "((volatile uint8_t *)fresh_entry_root_mapping)[3] = 0x90u"
    )
    for required in (
        "CONFIG_SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_PROBE",
        "((volatile uint8_t *)fresh_entry_root_mapping)[3] = 0x90u",
        "((volatile uint8_t *)fresh_entry_root_mapping)[4] = 0x0fu",
        "((volatile uint8_t *)fresh_entry_root_mapping)[5] = 0x0bu",
    ):
        require(required in payload, "missing Phase 77 payload control: " + required)

    phase77 = config_block_before(
        source,
        "seL4_SetMR(0u, SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_RESTART_VADDR)",
    )
    for required in (
        "SELINOS_FRESH_TARGET_FIRST_FETCH_M0_FIXED_POST_NOP_VADDR",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_FIRST_FAULT_VADDR",
        "seL4_SetMR(0u, SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_RESTART_VADDR)",
        "seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_REPLY_WORDS",
        "fault_message = seL4_Recv(fresh_fault_endpoint.cptr, &fault_badge)",
        "seL4_Fault_UserException",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_SECOND_FAULT_VADDR",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FIXED_STACK_POINTER",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_INVALID_OPCODE_VECTOR",
        "SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_M0_FAULT_BADGE",
    ):
        require(required in phase77, "missing Phase 77 control: " + required)
    require(phase77.count("seL4_Reply(") == 1, "Phase 77 must issue exactly one reply")
    for forbidden in (
        "seL4_TCB_Resume(fresh_tcb.cptr)",
        "seL4_TCB_WriteRegisters(fresh_tcb.cptr",
        "seL4_SetMR(1u,",
        "seL4_SetMR(2u,",
    ):
        require(forbidden not in phase77, "forbidden Phase 77 control: " + forbidden)
    require(source.count("seL4_TCB_Resume(fresh_tcb.cptr)") == 1, "combined path must resume once")

    print("SeLinOS fresh target fault reply/restart M0 evidence verified.")
except Exception as error:
    print("verification failed: " + str(error), file=sys.stderr)
    sys.exit(1)
