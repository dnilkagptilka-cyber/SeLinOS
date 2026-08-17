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
        (root / "tests/artifacts/selinos_memd_fixed_mapping_inventory_m1.verification.json").read_text()
    )
    need(record["schema"] == 1, "invalid memd M1 evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid memd M1 platform evidence")
    bind(root, record["image"], "image")
    root_wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    memd = bind(root, record["implementation"]["memd"], "memd").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_MEMD_M1_RESERVE_FIXED_PLAN",
        "SELINOS_MEMD_M1_RESERVED",
        "SELINOS_MEMD_M1_EBUSY",
        "SELINOS_MEMD_M1_EINVAL",
        "SELINOS_MEMD_M1_SLOT 1u",
        "SELINOS_MEMD_M1_ENDPOINT_SLOT 8u",
    ):
        need(required in protocol, f"missing protocol control: {required}")

    for required in (
        "start_memd_fixed_mapping_inventory_m1_bundle",
        "selinos-memd-m1-probe",
        "vka_alloc_endpoint(vka, &endpoint)",
        "memd_slot != SELINOS_MEMD_M1_ENDPOINT_SLOT",
        "probe_slot != SELINOS_MEMD_M1_PROBE_ENDPOINT_SLOT",
    ):
        need(required in root_wiring, f"missing root wiring control: {required}")

    for required in (
        "request_is_exact_reserve_fixed_plan",
        "bool reserved = false",
        "reply_status(SELINOS_MEMD_M1_RESERVED)",
        "reply_status(SELINOS_MEMD_M1_EBUSY)",
        "fixed mapping-plan reservation-state proof passed",
    ):
        need(required in memd, f"missing memd control: {required}")
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_Untyped_Retype",
        "seL4_TCB_", "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace",
        "SELINOS_LINUX_CLONE",
    ):
        need(forbidden not in memd, f"forbidden memd authority/control: {forbidden}")

    for required in (
        "call_reserve_expect(SELINOS_MEMD_M1_RESERVED)",
        "call_reserve_expect(SELINOS_MEMD_M1_EBUSY)",
        "one-slot fixed-plan reserve then EBUSY passed",
    ):
        need(required in probe, f"missing probe control: {required}")
    for forbidden in ("seL4_TCB_", "vka_", "vspace_", "SELINOS_LINUX_CLONE"):
        need(forbidden not in probe, f"forbidden probe authority/control: {forbidden}")

    for marker in record["runtime_evidence"]["required_markers"]:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")

    claims = " ".join(record["not_claimed"])
    for excluded in ("VSpace", "W^X", "capability lease", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS memd fixed mapping inventory M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
