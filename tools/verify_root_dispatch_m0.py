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
    record = json.loads((root / "tests/artifacts/selinos_root_dispatch_m0.verification.json").read_text())
    need(record["schema"] == 1, "invalid root-dispatch M0 evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid root-dispatch M0 platform evidence")
    bind(root, record["image"], "image")
    wiring = bind(root, record["implementation"]["root_wiring"], "root wiring").read_text()
    main_source = bind(root, record["implementation"]["root_main"], "root main").read_text()
    probe = bind(root, record["implementation"]["probe"], "probe").read_text()
    protocol = bind(root, record["implementation"]["protocol"], "protocol").read_text()
    runtime = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_ROOT_DISPATCH_M0_REQUEST",
        "SELINOS_ROOT_DISPATCH_M0_READY",
        "SELINOS_ROOT_DISPATCH_M0_SLOT 1u",
        "SELINOS_ROOT_DISPATCH_M0_GENERATION 1u",
        "SELINOS_ROOT_DISPATCH_M0_WORDS 3u",
        "SELINOS_ROOT_DISPATCH_M0_PROBE_ENDPOINT_SLOT 8u",
    ):
        need(required in protocol, f"missing protocol control: {required}")

    for required in (
        "static seL4_CPtr root_dispatch_m0_endpoint = seL4_CapNull;",
        "start_root_dispatch_m0_probe_bundle",
        "selinos-root-dispatch-m0-probe",
        "root_dispatch_m0_endpoint = endpoint.cptr;",
        "bool selinos_root_dispatch_m0_once(void)",
        "seL4_Recv(root_dispatch_m0_endpoint, &badge)",
        "SELINOS_ROOT_DISPATCH_M0_REQUEST",
        "SELINOS_ROOT_DISPATCH_M0_READY",
        "fixed status reply issued; no child constructed",
    ):
        need(required in wiring, f"missing root dispatch control: {required}")

    dispatch = wiring[wiring.index("bool selinos_root_dispatch_m0_once(void)"):wiring.index("/* Phase 33")]
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_Untyped_Retype",
        "seL4_TCB_", "seL4_SetCap(", "seL4_IRQControl", "seL4_X86_IOPort",
        "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE",
    ):
        need(forbidden not in dispatch, f"forbidden root-dispatch authority/control: {forbidden}")

    need("selinos_root_dispatch_m0_once()" in main_source, "main does not invoke root dispatch")
    need(main_source.index("selinos_root_dispatch_m0_once()") < main_source.index("root task entering idle/yield loop"),
         "root dispatch must precede idle loop")

    for required in (
        "seL4_Call(",
        "SELINOS_ROOT_DISPATCH_M0_PROBE_ENDPOINT_SLOT",
        "SELINOS_ROOT_DISPATCH_M0_READY",
        "fixed root status transaction passed",
    ):
        need(required in probe, f"missing probe control: {required}")
    for forbidden in ("vka_", "vspace_", "seL4_SetCap", "seL4_TCB_", "SELINOS_LINUX_"):
        need(forbidden not in probe, f"forbidden probe authority/control: {forbidden}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden marker: {marker}")
    ready_position = runtime.index(markers[0])
    accepted_position = runtime.index(markers[1])
    replied_position = runtime.index(markers[2])
    idle_position = runtime.index(markers[3])
    probe_position = runtime.index(markers[4])
    need(ready_position < accepted_position < replied_position < idle_position,
         "root-side root-dispatch M0 markers are not ordered")
    need(replied_position < probe_position,
         "root-dispatch probe observed success before root status reply")

    claims = " ".join(record["not_claimed"])
    for excluded in ("root child construction", "dynamic allocation", "Linux clone", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS root construction-request dispatch M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
