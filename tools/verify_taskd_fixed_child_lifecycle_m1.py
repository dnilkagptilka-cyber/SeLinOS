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
    record = json.loads(
        (root / "tests/artifacts/selinos_taskd_fixed_child_lifecycle_m1.verification.json").read_text()
    )
    need(record["schema"] == 1, "invalid taskd M1 evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid taskd M1 platform evidence")
    bind(root, record["image"], "image")
    root_wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    taskd = bind(root, record["implementation"]["taskd"], "taskd").read_text()
    child = bind(root, record["implementation"]["child"], "child").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_TASKD_M1_START",
        "SELINOS_TASKD_M1_CHILD_DONE",
        "SELINOS_TASKD_M1_SUCCESS",
        "SELINOS_TASKD_M1_CHILD_ID 1u",
        "SELINOS_TASKD_M1_CLIENT_ENDPOINT_SLOT 8u",
        "SELINOS_TASKD_M1_CHILD_TCB_SLOT 11u",
        "SELINOS_TASKD_M1_PROBE_SUCCESS_SLOT 12u",
    ):
        need(required in protocol, f"missing pinned protocol control: {required}")

    for required in (
        "start_taskd_fixed_child_lifecycle_m1_bundle",
        "selinos-task-lifecycle-child",
        "selinos-task-lifecycle-probe",
        "vka_alloc_endpoint(vka, &client_endpoint)",
        "vka_alloc_endpoint(vka, &child_completion_endpoint)",
        "vka_alloc_notification(vka, &child_start_notification)",
        "vka_alloc_notification(vka, &probe_success_notification)",
        "child.thread.tcb.cptr",
        "taskd_child_tcb_slot != SELINOS_TASKD_M1_CHILD_TCB_SLOT",
    ):
        need(required in root_wiring, f"missing root wiring control: {required}")

    for forbidden in ("SELINOS_LINUX_CLONE", "seL4_TCB_Suspend(taskd.", "seL4_TCB_Resume(taskd."):
        need(forbidden not in root_wiring, f"forbidden root lifecycle control: {forbidden}")

    for required in (
        "receive_exact_client_start",
        "receive_exact_child_completion",
        "seL4_TCB_Suspend(SELINOS_TASKD_M1_CHILD_TCB_SLOT)",
        "seL4_TCB_Resume(SELINOS_TASKD_M1_CHILD_TCB_SLOT)",
        "seL4_Signal(SELINOS_TASKD_M1_CHILD_START_SLOT)",
        "seL4_Signal(SELINOS_TASKD_M1_PROBE_SUCCESS_SLOT)",
    ):
        need(required in taskd, f"missing taskd lifecycle control: {required}")

    for forbidden in (
        "vspace_new_pages", "sel4utils_configure_process", "seL4_Untyped_Retype",
        "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE",
    ):
        need(forbidden not in taskd, f"forbidden taskd authority/control: {forbidden}")

    for required in (
        "seL4_Wait(SELINOS_TASKD_M1_CHILD_START_WAIT_SLOT, &badge)",
        "send_completion(SELINOS_TASKD_M1_FIRST_GENERATION)",
        "send_completion(SELINOS_TASKD_M1_SECOND_GENERATION)",
    ):
        need(required in child, f"missing child lifecycle control: {required}")
    for forbidden in ("syscall", "SELINOS_LINUX_", "seL4_TCB_", "vspace_"):
        need(forbidden not in child, f"forbidden child authority/control: {forbidden}")

    for required in (
        "seL4_Send(SELINOS_TASKD_M1_PROBE_CLIENT_ENDPOINT_SLOT",
        "seL4_Wait(SELINOS_TASKD_M1_PROBE_SUCCESS_WAIT_SLOT, &badge)",
        "fixed child taskd lifecycle passed",
    ):
        need(required in probe, f"missing probe control: {required}")
    for forbidden in ("seL4_TCB_", "SELINOS_LINUX_CLONE", "vspace_"):
        need(forbidden not in probe, f"forbidden probe authority/control: {forbidden}")

    for marker in record["runtime_evidence"]["required_markers"]:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")

    claims = " ".join(record["not_claimed"])
    for excluded in ("Linux clone", "dynamic child", "futex exit", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS taskd fixed-child lifecycle M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
