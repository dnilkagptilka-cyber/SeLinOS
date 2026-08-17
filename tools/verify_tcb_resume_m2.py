#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def bind(root: Path, item: dict, label: str) -> Path:
    path = root / item["path"]
    need(path.is_file(), f"missing {label}")
    need(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / "tests/artifacts/selinos_tcb_resume_m2.verification.json").read_text())
    need(record["schema"] == 1, "invalid TCB resume M2 evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid TCB resume M2 platform evidence")
    bind(root, record["image"], "image")
    root_wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    taskd = bind(root, record["implementation"]["taskd"], "taskd").read_text()
    objectd = bind(root, record["implementation"]["objectd"], "objectd").read_text()
    child = bind(root, record["implementation"]["child"], "child").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_TCB_RESUME_M2_REQUEST",
        "SELINOS_TCB_RESUME_M2_GRANTED",
        "SELINOS_TCB_RESUME_M2_CHILD_RAN",
        "SELINOS_TCB_RESUME_M2_SLOT 1u",
        "SELINOS_TCB_RESUME_M2_GENERATION 1u",
        "SELINOS_TCB_RESUME_M2_TASKD_TCB_DEST_SLOT 12u",
        "SELINOS_TCB_RESUME_M2_OBJECT_TCB_SOURCE_SLOT 9u",
    ):
        need(required in protocol, f"missing protocol control: {required}")

    for required in (
        "start_transferred_tcb_single_resume_m2_bundle",
        "selinos-tcb-resume-m2-child",
        "sel4utils_spawn_process_v(&child, vka, vspace, 1, child_argv, 0)",
        "objectd_tcb_slot = sel4utils_copy_cap_to_process(&objectd, vka, child.thread.tcb.cptr)",
        "fresh child spawned suspended for one taskd resume",
    ):
        need(required in root_wiring, f"missing root wiring control: {required}")

    for required in (
        "seL4_SetCapReceivePath(SELINOS_TCB_RESUME_M2_TASKD_CNODE_SLOT",
        "SELINOS_TCB_RESUME_M2_TASKD_TCB_DEST_SLOT",
        "seL4_TCB_Resume(SELINOS_TCB_RESUME_M2_TASKD_TCB_DEST_SLOT)",
        "receive_exact_child_completion",
        "exact one child completion validated",
    ):
        need(required in taskd, f"missing taskd control: {required}")
    need(taskd.count("seL4_TCB_") == 1, "taskd must contain exactly one TCB invocation")

    for required in (
        "seL4_SetCap(0, SELINOS_TCB_RESUME_M2_OBJECT_TCB_SOURCE_SLOT)",
        "suspended child TCB cap transferred to taskd",
    ):
        need(required in objectd, f"missing objectd transfer control: {required}")
    need("seL4_TCB_" not in objectd, "objectd must not invoke the child TCB")

    for required in (
        "SELINOS_TCB_RESUME_M2_CHILD_RAN",
        "SELINOS_TCB_RESUME_M2_CHILD_COMPLETION_SEND_SLOT",
        "one fixed completion sent",
    ):
        need(required in child, f"missing child completion control: {required}")
    for forbidden in ("seL4_TCB_", "vka_", "vspace_", "SELINOS_LINUX_"):
        need(forbidden not in child, f"forbidden child authority/control: {forbidden}")

    for source_name, source in (("taskd", taskd), ("objectd", objectd), ("child", child), ("probe", probe)):
        for forbidden in (
            "vka_", "vspace_", "sel4utils_configure_process", "seL4_Untyped_Retype",
            "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE",
        ):
            need(forbidden not in source, f"forbidden {source_name} authority/control: {forbidden}")

    for required in (
        "seL4_Send(SELINOS_TCB_RESUME_M2_PROBE_CLIENT_ENDPOINT_SLOT",
        "seL4_Wait(SELINOS_TCB_RESUME_M2_PROBE_SUCCESS_NOTIFY_SLOT, &badge)",
        "one transferred child resume passed",
    ):
        need(required in probe, f"missing probe control: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "TCB resume M2 markers are not ordered")

    claims = " ".join(record["not_claimed"])
    for excluded in ("dynamic child", "second resume", "W^X", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS transferred child TCB single-resume M2 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
