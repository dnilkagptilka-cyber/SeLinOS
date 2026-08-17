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
    record = json.loads((root / "tests/artifacts/selinos_root_construct_m3.verification.json").read_text())
    need(record["schema"] == 1, "invalid root-construction M3 evidence schema")
    need(record["platform"]["architecture"] == "x86_64 PC99", "wrong M3 platform")
    need(record["platform"]["linux_kernel_present"] is False, "Linux kernel invalidates M3 evidence")
    configuration = record["platform"]["configuration"]
    for required in ("SeLinRootConstructM1Adapter=ON", "SeLinRootConstructM2ControlLease=ON",
                     "SeLinRootConstructM3SingleResume=ON"):
        need(required in configuration, f"missing M3 profile setting: {required}")

    image = bind(root, record["image"], "M3 image")
    need(len(image) > 0, "empty M3 image")
    m1 = bind(root, record["implementation"]["root_m1_adapter"], "root M1 adapter")
    m2 = bind(root, record["implementation"]["root_m2_lease"], "root M2 lease")
    wiring = bind(root, record["implementation"]["root_wiring"], "root wiring")
    root_main = bind(root, record["implementation"]["root_main"], "root main")
    client = bind(root, record["implementation"]["taskd_client"], "M3 taskd client")
    child = bind(root, record["implementation"]["witness_child"], "M3 witness child")
    m1_protocol = bind(root, record["implementation"]["m1_protocol"], "M1 protocol")
    m2_protocol = bind(root, record["implementation"]["m2_protocol"], "M2 protocol")
    m3_protocol = bind(root, record["implementation"]["m3_protocol"], "M3 protocol")
    cmake = bind(root, record["implementation"]["build_gate"], "M3 build gate")
    runtime = bind(root, record["runtime_evidence"], "M3 runtime log")

    need(record["image"]["path"].startswith("build-root-construct-m3/"),
         "M3 must bind a distinct triple-gated proof image")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST", "SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED",
        "SELINOS_ROOT_CONSTRUCT_M1_REJECTED", "SELINOS_ROOT_CONSTRUCT_M1_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M1_GENERATION 1u",
    ):
        need(required in m1_protocol, f"missing M1 provenance control: {required}")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M2_LEASE_TCB", "SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED", "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS 1u",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS 0u",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT 9u",
    ):
        need(required in m2_protocol, f"missing M2 provenance control: {required}")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M3_CHILD_TCB_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M2_TASKD_TCB_DEST_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M3_RESUME_COUNT 1u",
    ):
        need(required in m3_protocol, f"missing M3 fixed-action control: {required}")
    for excluded in ("image", "stack", "register", "TLS", "clone", "scheduler",
                     "credential", "VFS", "device parameter"):
        need(excluded in m3_protocol, f"M3 protocol omission is not documented: {excluded}")

    for required in (
        "SeLinRootConstructM3SingleResume", "SELINOS_ROOT_CONSTRUCT_M3_SINGLE_RESUME",
        "requires SeLinRootConstructM2ControlLease", "selinos-taskd-root-construct-m3",
        "selinos-root-construct-m3-witness-child", "if(SeLinRootConstructM3SingleResume)",
    ):
        need(required in cmake, f"missing M3 build-gate control: {required}")

    for required in (
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M3_SINGLE_RESUME",
        "selinos-root-construct-m3-witness-child",
        "selinos-taskd-root-construct-m3",
        "sel4utils_spawn_process_v(&child, root_construct_m1_vka,",
        "root_construct_m1_vspace, 1, child_argv, 0)",
        "selinos_root_construct_m2_record_child_tcb(child.thread.tcb.cptr)",
    ):
        need(required in m1, f"missing M3 root provenance/selection control: {required}")
    need(m1.index("sel4utils_spawn_process_v(&child") <
         m1.index("selinos_root_construct_m2_record_child_tcb"),
         "M2 provenance must follow suspended child spawn")
    for forbidden in ("seL4_TCB_", "seL4_Untyped_Retype", "seL4_IRQControl",
                      "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE"):
        need(forbidden not in m1, f"forbidden root M1 control: {forbidden}")

    for required in (
        "root_construct_m2_lease_granted = true;",
        "seL4_SetCap(0u, root_construct_m2_child_tcb);",
        "seL4_SetCap(0u, seL4_CapNull);",
        "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS",
    ):
        need(required in m2, f"missing M2 one-cap control: {required}")
    for forbidden in ("seL4_TCB_", "sel4utils_configure_process", "sel4utils_spawn_process_v",
                      "vka_", "vspace_", "seL4_Untyped_Retype", "seL4_IRQControl",
                      "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE"):
        need(forbidden not in m2, f"forbidden M2 root control: {forbidden}")

    for required in (
        "call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED)",
        "call_root_construct_m1(SELINOS_ROOT_CONSTRUCT_M1_REJECTED)",
        "seL4_SetCapReceivePath(SELINOS_ROOT_CONSTRUCT_M2_TASKD_CNODE_SLOT,",
        "SELINOS_ROOT_CONSTRUCT_M2_LEASE_GRANTED",
        "SELINOS_ROOT_CONSTRUCT_M2_GRANTED_CAPS",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED",
        "SELINOS_ROOT_CONSTRUCT_M2_REJECTED_CAPS",
        "seL4_TCB_Resume(SELINOS_ROOT_CONSTRUCT_M3_CHILD_TCB_SLOT)",
        "!= seL4_NoError",
        "invoking exactly one received-child TCB resume",
        "one received-child TCB resume returned success",
    ):
        need(required in client, f"missing M3 client control: {required}")
    need(client.count("seL4_TCB_Resume") == 1,
         "M3 taskd client must contain exactly one resume operation")
    for forbidden in (
        "seL4_TCB_Suspend", "seL4_TCB_SetPriority", "seL4_TCB_SetMCPriority",
        "seL4_TCB_SetAffinity", "seL4_TCB_WriteRegisters", "seL4_TCB_SetTLSBase",
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_SetCap(",
        "seL4_Untyped_Retype", "seL4_IRQControl", "seL4_X86_IOPort",
        "seL4_X86_IOSpace", "SELINOS_LINUX_",
    ):
        need(forbidden not in client, f"forbidden M3 client authority/control: {forbidden}")

    need("executed once after taskd resume" in child, "witness child lacks execution marker")
    for forbidden in (
        "seL4_TCB_", "seL4_Call", "seL4_Recv", "seL4_Reply", "seL4_Signal",
        "vka_", "vspace_", "SELINOS_LINUX_", "sel4utils_",
    ):
        need(forbidden not in child, f"forbidden M3 child authority/control: {forbidden}")

    for required in (
        "selinos_root_construct_m1_start_bundle(vka, vspace)",
        "selinos_root_construct_m1_dispatch_loop();",
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER",
    ):
        need(required in wiring + root_main, f"missing root M3 branch prerequisite: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing required runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden runtime marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "M3 runtime markers are not ordered")

    non_claims = " ".join(record["not_claimed"])
    for excluded in ("second resume", "Linux clone", "dpkg or apt", "teardown"):
        need(excluded in non_claims, f"missing M3 non-claim: {excluded}")

    print("SeLinOS root-constructed child single-resume M3 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
