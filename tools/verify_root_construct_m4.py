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
    record = json.loads((root / "tests/artifacts/selinos_root_construct_m4.verification.json").read_text())
    need(record["schema"] == 1, "invalid root-construction M4 evidence schema")
    need(record["platform"]["architecture"] == "x86_64 PC99", "wrong M4 platform")
    need(record["platform"]["linux_kernel_present"] is False, "Linux kernel invalidates M4 evidence")
    configuration = record["platform"]["configuration"]
    for required in ("SeLinRootConstructM1Adapter=ON", "SeLinRootConstructM2ControlLease=ON",
                     "SeLinRootConstructM3SingleResume=ON", "SeLinRootConstructM4Completion=ON"):
        need(required in configuration, f"missing M4 profile setting: {required}")

    image = bind(root, record["image"], "M4 image")
    need(len(image) > 0, "empty M4 image")
    m1 = bind(root, record["implementation"]["root_m1_adapter"], "root M1 adapter")
    m2 = bind(root, record["implementation"]["root_m2_lease"], "root M2 lease")
    m4 = bind(root, record["implementation"]["root_m4_endpoint"], "root M4 endpoint")
    wiring = bind(root, record["implementation"]["root_wiring"], "root wiring")
    root_main = bind(root, record["implementation"]["root_main"], "root main")
    client = bind(root, record["implementation"]["taskd_client"], "M4 taskd client")
    child = bind(root, record["implementation"]["completion_child"], "M4 completion child")
    m1_protocol = bind(root, record["implementation"]["m1_protocol"], "M1 protocol")
    m2_protocol = bind(root, record["implementation"]["m2_protocol"], "M2 protocol")
    m3_protocol = bind(root, record["implementation"]["m3_protocol"], "M3 protocol")
    m4_protocol = bind(root, record["implementation"]["m4_protocol"], "M4 protocol")
    cmake = bind(root, record["implementation"]["build_gate"], "M4 build gate")
    runtime = bind(root, record["runtime_evidence"], "M4 runtime log")

    need(record["image"]["path"].startswith("build-root-construct-m4/"),
         "M4 must bind a distinct four-gated proof image")
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
    ):
        need(required in m2_protocol, f"missing M2 provenance control: {required}")
    for required in ("SELINOS_ROOT_CONSTRUCT_M3_CHILD_TCB_SLOT",
                     "SELINOS_ROOT_CONSTRUCT_M3_RESUME_COUNT 1u"):
        need(required in m3_protocol, f"missing M3 fixed resume control: {required}")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE",
        "SELINOS_ROOT_CONSTRUCT_M4_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M4_GENERATION 1u",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS 3u",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT 8u",
        "SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT 9u",
        "SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT 10u",
    ):
        need(required in m4_protocol, f"missing M4 closed protocol control: {required}")
    for excluded in ("image", "stack", "register", "TLS", "clone", "scheduler",
                     "credential", "VFS", "device parameter"):
        need(excluded in m4_protocol, f"M4 protocol omission is not documented: {excluded}")

    for required in (
        "SeLinRootConstructM4Completion", "SELINOS_ROOT_CONSTRUCT_M4_COMPLETION",
        "requires SeLinRootConstructM3SingleResume", "selinos-taskd-root-construct-m4",
        "selinos-root-construct-m4-completion-child", "src/root_construct_m4.c",
        "if(SeLinRootConstructM4Completion)",
    ):
        need(required in cmake, f"missing M4 build-gate control: {required}")

    for required in (
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M4_COMPLETION",
        "selinos-root-construct-m4-completion-child",
        "selinos-taskd-root-construct-m4",
        "selinos_root_construct_m4_prepare_taskd(&taskd_client, root_vka)",
        "selinos_root_construct_m4_prepare_child(&child, root_construct_m1_vka)",
        "sel4utils_spawn_process_v(&child, root_construct_m1_vka,",
        "root_construct_m1_vspace, 1, child_argv, 0)",
        "selinos_root_construct_m2_record_child_tcb(child.thread.tcb.cptr)",
    ):
        need(required in m1, f"missing M4 root construction control: {required}")
    need(m1.index("selinos_root_construct_m4_prepare_child") <
         m1.index("sel4utils_spawn_process_v(&child"),
         "M4 child endpoint must be copied before suspended spawn")
    need(m1.index("sel4utils_spawn_process_v(&child") <
         m1.index("selinos_root_construct_m2_record_child_tcb"),
         "M2 provenance must follow suspended child spawn")
    for forbidden in ("seL4_TCB_", "seL4_Untyped_Retype", "seL4_IRQControl",
                      "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE"):
        need(forbidden not in m1, f"forbidden root M1 control: {forbidden}")

    for required in (
        "vka_alloc_endpoint(root_vka, &root_construct_m4_completion_endpoint)",
        "SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT",
        "sel4utils_copy_cap_to_process",
        "root_construct_m4_taskd_prepared = true;",
        "root_construct_m4_child_prepared = true;",
    ):
        need(required in m4, f"missing root M4 endpoint control: {required}")
    need(m4.count("vka_alloc_endpoint") == 1, "M4 must allocate exactly one endpoint")
    need(m4.count("sel4utils_copy_cap_to_process") == 2,
         "M4 must copy the endpoint only to taskd and the one child")
    for forbidden in ("seL4_TCB_", "sel4utils_configure_process", "sel4utils_spawn_process_v",
                      "seL4_Untyped_Retype", "seL4_IRQControl", "seL4_X86_IOPort",
                      "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE", "vka_free", "vspace_"):
        need(forbidden not in m4, f"forbidden M4 root authority/control: {forbidden}")

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
        "SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT",
        "seL4_TCB_Resume(SELINOS_ROOT_CONSTRUCT_M4_TASKD_TCB_DEST_SLOT)",
        "seL4_Recv(\n        SELINOS_ROOT_CONSTRUCT_M4_TASKD_COMPLETION_ENDPOINT_SLOT, &badge)",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS",
        "one received-child TCB resumed; waiting for completion",
        "exact one child completion validated",
    ):
        need(required in client, f"missing M4 taskd client control: {required}")
    need(client.count("seL4_TCB_Resume") == 1, "M4 taskd client must contain exactly one resume")
    need(client.count("seL4_Recv") == 1, "M4 taskd client must contain exactly one completion receive")
    for forbidden in (
        "seL4_TCB_Suspend", "seL4_TCB_SetPriority", "seL4_TCB_SetMCPriority",
        "seL4_TCB_SetAffinity", "seL4_TCB_WriteRegisters", "seL4_TCB_SetTLSBase",
        "seL4_Send", "seL4_Signal", "vka_", "vspace_", "sel4utils_configure_process",
        "seL4_SetCap(", "seL4_Untyped_Retype", "seL4_IRQControl", "seL4_X86_IOPort",
        "seL4_X86_IOSpace", "SELINOS_LINUX_",
    ):
        need(forbidden not in client, f"forbidden M4 client authority/control: {forbidden}")

    for required in (
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_COMPLETION_ENDPOINT_SLOT",
        "seL4_Send(", "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE",
        "SELINOS_ROOT_CONSTRUCT_M4_CHILD_DONE_WORDS", "one exact completion sent",
    ):
        need(required in child, f"missing M4 child completion control: {required}")
    need(child.count("seL4_Send") == 1, "M4 child must send exactly one completion")
    for forbidden in (
        "seL4_TCB_", "seL4_Call", "seL4_Recv", "seL4_Reply", "seL4_Signal",
        "vka_", "vspace_", "SELINOS_LINUX_", "sel4utils_",
    ):
        need(forbidden not in child, f"forbidden M4 child authority/control: {forbidden}")

    for required in (
        "selinos_root_construct_m1_start_bundle(vka, vspace)",
        "selinos_root_construct_m1_dispatch_loop();",
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER",
    ):
        need(required in wiring + root_main, f"missing root M4 branch prerequisite: {required}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing required runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden runtime marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "M4 runtime markers are not ordered")

    non_claims = " ".join(record["not_claimed"])
    for excluded in ("general IPC namespace", "second resume", "Linux clone", "dpkg or apt"):
        need(excluded in non_claims, f"missing M4 non-claim: {excluded}")

    print("SeLinOS root-constructed child completion witness M4 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
