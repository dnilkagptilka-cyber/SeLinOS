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


def bind(root: Path, item: dict, label: str) -> str:
    path = root / item["path"]
    need(path.is_file(), f"missing {label}")
    need(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path.read_text(errors="replace")


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / "tests/artifacts/selinos_root_construct_m2.verification.json").read_text())
    need(record["schema"] == 1, "invalid root-construction M2 evidence schema")
    need(record["platform"]["architecture"] == "x86_64 PC99", "wrong M2 platform")
    need(record["platform"]["linux_kernel_present"] is False, "Linux kernel invalidates M2 evidence")
    configuration = record["platform"]["configuration"]
    for required in ("SeLinRootConstructM1Adapter=ON", "SeLinRootConstructM2ControlLease=ON"):
        need(required in configuration, f"missing M2 profile setting: {required}")

    image = bind(root, record["image"], "M2 image")
    need(len(image) > 0, "empty M2 image")
    m1 = bind(root, record["implementation"]["root_m1_adapter"], "root M1 adapter")
    m2 = bind(root, record["implementation"]["root_m2_lease"], "root M2 lease")
    wiring = bind(root, record["implementation"]["root_wiring"], "root wiring")
    root_main = bind(root, record["implementation"]["root_main"], "root main")
    client = bind(root, record["implementation"]["taskd_client"], "M2 taskd client")
    child = bind(root, record["implementation"]["child"], "inert child")
    m1_protocol = bind(root, record["implementation"]["m1_protocol"], "M1 protocol")
    m2_protocol = bind(root, record["implementation"]["m2_protocol"], "M2 protocol")
    cmake = bind(root, record["implementation"]["build_gate"], "M2 build gate")
    runtime = bind(root, record["runtime_evidence"], "M2 runtime log")

    need(record["image"]["path"].startswith("build-root-construct-m2/"),
         "M2 must bind a distinct dual-gated proof image")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST", "SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED",
        "SELINOS_ROOT_CONSTRUCT_M1_REJECTED", "SELINOS_ROOT_CONSTRUCT_M1_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M1_GENERATION 1u",
    ):
        need(required in m1_protocol, f"missing M1 provenance control: {required}")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M2_LEASE_TCB",
        "SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED",
        "SELINOS_ROOT_CONSTRUCT_M2_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M2_GENERATION 1u",
        "SELINOS_ROOT_CONSTRUCT_M2_REQUEST_WORDS 3u",
        "SELINOS_ROOT_CONSTRUCT_M2_REPLY_WORDS 3u",
        "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS 1u",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS 0u",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_REQUEST_ENDPOINT_SLOT 8u",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_CNODE_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT 9u",
    ):
        need(required in m2_protocol, f"missing M2 closed protocol control: {required}")
    for excluded in ("pointer", "image", "stack", "register", "TLS", "clone",
                     "scheduler", "credential", "VFS", "device parameter"):
        need(excluded in m2_protocol, f"M2 protocol omission is not documented: {excluded}")

    for required in (
        "SeLinRootConstructM2ControlLease",
        "SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE",
        "requires SeLinRootConstructM1Adapter",
        "selinos-taskd-root-construct-m2",
        "src/root_construct_m2.c",
        "if(SeLinRootConstructM2ControlLease)",
    ):
        need(required in cmake, f"missing M2 build-gate control: {required}")

    for required in (
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE",
        "selinos_root_construct_m2_record_child_tcb(child.thread.tcb.cptr)",
        "selinos_root_construct_m2_try_dispatch(badge, message)",
        "sel4utils_spawn_process_v(&child, root_construct_m1_vka,",
        "root_construct_m1_vspace, 1, child_argv, 0)",
    ):
        need(required in m1, f"missing M1-to-M2 provenance control: {required}")
    need(m1.index("sel4utils_spawn_process_v(&child") <
         m1.index("selinos_root_construct_m2_record_child_tcb"),
         "M2 provenance must be recorded only after suspended spawn")

    for required in (
        "static seL4_CPtr root_construct_m2_child_tcb = seL4_CapNull;",
        "static bool root_construct_m2_lease_granted;",
        "root_construct_m2_child_tcb = child_tcb;",
        "root_construct_m2_lease_granted = true;",
        "seL4_SetCap(0u, root_construct_m2_child_tcb);",
        "seL4_SetCap(0u, seL4_CapNull);",
        "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS",
        "malformed, unavailable or duplicate lease rejected; no cap delivered",
        "one constructed-child TCB control cap delivered; child remains suspended",
    ):
        need(required in m2, f"missing root M2 lease control: {required}")
    need(m2.count("seL4_SetCap") == 2, "M2 must set only the one reply-cap slot and its clear path")
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "sel4utils_spawn_process_v",
        "seL4_TCB_", "seL4_Untyped_Retype", "seL4_IRQControl", "seL4_X86_IOPort",
        "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE", "vka_free", "vspace_free",
    ):
        need(forbidden not in m2, f"forbidden root M2 authority/control: {forbidden}")

    for required in (
        "call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED)",
        "call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED)",
        "seL4_SetCapReceivePath(SELINOS_ROOT_CONSTRUCT_M2_TASKD_CNODE_SLOT,",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED",
        "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS",
        "one provenance TCB cap received and not exercised; duplicate refused",
    ):
        need(required in client, f"missing M2 taskd-client control: {required}")
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_SetCap(",
        "seL4_TCB_", "seL4_Untyped_Retype", "seL4_IRQControl", "seL4_X86_IOPort",
        "seL4_X86_IOSpace", "SELINOS_LINUX_",
    ):
        need(forbidden not in client, f"forbidden M2 client authority/control: {forbidden}")
    need("unexpected execution" in child, "inert child lacks execution-failure sentinel")
    for forbidden in ("vka_", "vspace_", "seL4_TCB_", "SELINOS_LINUX_"):
        need(forbidden not in child, f"forbidden inert-child authority/control: {forbidden}")

    for required in (
        "selinos_root_construct_m1_start_bundle(vka, vspace)",
        "selinos_root_construct_m1_dispatch_loop();",
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER",
    ):
        need(required in wiring + root_main, f"missing root M2 branch prerequisite: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing required runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden runtime marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "M2 runtime markers are not ordered")

    non_claims = " ".join(record["not_claimed"])
    for excluded in ("child start", "second cap transfer", "Linux clone", "dpkg or apt"):
        need(excluded in non_claims, f"missing M2 non-claim: {excluded}")

    print("SeLinOS root-constructed child control-lease M2 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
