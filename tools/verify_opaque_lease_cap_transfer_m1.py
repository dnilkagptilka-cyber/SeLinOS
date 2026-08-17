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
    record = json.loads((root / "tests/artifacts/selinos_opaque_lease_cap_transfer_m1.verification.json").read_text())
    need(record["schema"] == 1, "invalid opaque lease evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid opaque lease platform evidence")
    bind(root, record["image"], "image")
    root_wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    taskd = bind(root, record["implementation"]["taskd"], "taskd").read_text()
    objectd = bind(root, record["implementation"]["objectd"], "objectd").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_OPAQUE_LEASE_M1_REQUEST",
        "SELINOS_OPAQUE_LEASE_M1_GRANTED",
        "SELINOS_OPAQUE_LEASE_M1_SLOT 1u",
        "SELINOS_OPAQUE_LEASE_M1_REPLY_CAPS 1u",
        "SELINOS_OPAQUE_LEASE_M1_TASKD_TOKEN_DEST_SLOT 11u",
        "SELINOS_OPAQUE_LEASE_M1_OBJECT_TOKEN_SOURCE_SLOT 9u",
    ):
        need(required in protocol, f"missing protocol control: {required}")

    for required in (
        "start_opaque_lease_m1_bundle",
        "selinos-taskd-opaque-lease-m1",
        "selinos-objectd-opaque-lease-m1",
        "selinos-taskd-opaque-lease-m1-probe",
        "vka_alloc_notification(vka, &opaque_token)",
        "objectd_token_slot != SELINOS_OPAQUE_LEASE_M1_OBJECT_TOKEN_SOURCE_SLOT",
        "taskd_success_slot != SELINOS_OPAQUE_LEASE_M1_TASKD_SUCCESS_NOTIFY_SLOT",
    ):
        need(required in root_wiring, f"missing root wiring control: {required}")

    for required in (
        "seL4_SetCap(0, SELINOS_OPAQUE_LEASE_M1_OBJECT_TOKEN_SOURCE_SLOT)",
        "SELINOS_OPAQUE_LEASE_M1_REPLY_CAPS",
        "one notification token transferred to taskd",
    ):
        need(required in objectd, f"missing objectd transfer control: {required}")

    for required in (
        "seL4_SetCapReceivePath(SELINOS_OPAQUE_LEASE_M1_TASKD_CNODE_SLOT",
        "SELINOS_OPAQUE_LEASE_M1_TASKD_TOKEN_DEST_SLOT",
        "seL4_MessageInfo_get_extraCaps(reply) != SELINOS_OPAQUE_LEASE_M1_REPLY_CAPS",
        "opaque notification token received; no resource created",
    ):
        need(required in taskd, f"missing taskd receive control: {required}")

    for source_name, source in (("taskd", taskd), ("objectd", objectd), ("probe", probe)):
        for forbidden in (
            "vka_", "vspace_", "sel4utils_configure_process", "seL4_Untyped_Retype",
            "seL4_TCB_", "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace",
            "SELINOS_LINUX_CLONE",
        ):
            need(forbidden not in source, f"forbidden {source_name} authority/control: {forbidden}")

    for required in (
        "seL4_Send(SELINOS_OPAQUE_LEASE_M1_PROBE_CLIENT_ENDPOINT_SLOT",
        "seL4_Wait(SELINOS_OPAQUE_LEASE_M1_PROBE_SUCCESS_NOTIFY_SLOT, &badge)",
        "fixed token transfer passed",
    ):
        need(required in probe, f"missing probe control: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "opaque lease markers are not ordered")

    claims = " ".join(record["not_claimed"])
    for excluded in ("actual resource", "dynamic child", "W^X", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS opaque lease capability transfer M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
