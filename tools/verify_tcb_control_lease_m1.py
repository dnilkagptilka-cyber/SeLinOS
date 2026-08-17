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
    record = json.loads((root / "tests/artifacts/selinos_tcb_control_lease_m1.verification.json").read_text())
    need(record["schema"] == 1, "invalid TCB lease evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid TCB lease platform evidence")
    bind(root, record["image"], "image")
    root_wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    taskd = bind(root, record["implementation"]["taskd"], "taskd").read_text()
    objectd = bind(root, record["implementation"]["objectd"], "objectd").read_text()
    child = bind(root, record["implementation"]["suspended_child"], "suspended child").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_TCB_LEASE_M1_REQUEST",
        "SELINOS_TCB_LEASE_M1_GRANTED",
        "SELINOS_TCB_LEASE_M1_SLOT 1u",
        "SELINOS_TCB_LEASE_M1_REPLY_CAPS 1u",
        "SELINOS_TCB_LEASE_M1_TASKD_TCB_DEST_SLOT 11u",
        "SELINOS_TCB_LEASE_M1_OBJECT_TCB_SOURCE_SLOT 9u",
    ):
        need(required in protocol, f"missing protocol control: {required}")

    for required in (
        "start_tcb_control_lease_m1_bundle",
        "selinos-tcb-lease-suspended-child",
        "selinos-taskd-tcb-lease-m1",
        "selinos-objectd-tcb-lease-m1",
        "selinos-taskd-tcb-lease-m1-probe",
        "objectd_tcb_slot = sel4utils_copy_cap_to_process(&objectd, vka, child.thread.tcb.cptr)",
        "dedicated child fixture configured and deliberately suspended",
    ):
        need(required in root_wiring, f"missing root wiring control: {required}")
    lease_block = root_wiring[root_wiring.index("start_tcb_control_lease_m1_bundle"):root_wiring.index("/* Phase 31")]
    need("sel4utils_spawn_process_v(&child" not in lease_block, "suspended child must not be spawned")

    for required in (
        "seL4_SetCap(0, SELINOS_TCB_LEASE_M1_OBJECT_TCB_SOURCE_SLOT)",
        "SELINOS_TCB_LEASE_M1_REPLY_CAPS",
        "suspended child TCB cap transferred to taskd",
    ):
        need(required in objectd, f"missing objectd transfer control: {required}")
    for required in (
        "seL4_SetCapReceivePath(SELINOS_TCB_LEASE_M1_TASKD_CNODE_SLOT",
        "SELINOS_TCB_LEASE_M1_TASKD_TCB_DEST_SLOT",
        "seL4_MessageInfo_get_extraCaps(reply) != SELINOS_TCB_LEASE_M1_REPLY_CAPS",
        "suspended child TCB cap received and not exercised",
    ):
        need(required in taskd, f"missing taskd receive control: {required}")

    for source_name, source in (("taskd", taskd), ("objectd", objectd), ("probe", probe)):
        for forbidden in (
            "vka_", "vspace_", "sel4utils_configure_process", "seL4_Untyped_Retype",
            "seL4_TCB_", "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace",
            "SELINOS_LINUX_CLONE",
        ):
            need(forbidden not in source, f"forbidden {source_name} authority/control: {forbidden}")
    need("unexpected execution" in child, "suspended child sentinel missing")

    for required in (
        "seL4_Send(SELINOS_TCB_LEASE_M1_PROBE_CLIENT_ENDPOINT_SLOT",
        "seL4_Wait(SELINOS_TCB_LEASE_M1_PROBE_SUCCESS_NOTIFY_SLOT, &badge)",
        "suspended child TCB transfer passed",
    ):
        need(required in probe, f"missing probe control: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "TCB lease markers are not ordered")

    claims = " ".join(record["not_claimed"])
    for excluded in ("TCB allocation", "dynamic child", "W^X", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS suspended child TCB control lease M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
