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


def source_slice(source: str, begin: str, end: str) -> str:
    start = source.index(begin)
    finish = source.index(end, start)
    return source[start:finish]


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / "tests/artifacts/selinos_root_construct_m1.verification.json").read_text())
    need(record["schema"] == 1, "invalid root-construction M1 evidence schema")
    need(record["platform"]["architecture"] == "x86_64 PC99", "wrong M1 platform")
    need(record["platform"]["linux_kernel_present"] is False, "Linux kernel invalidates M1 evidence")
    need("SeLinRootConstructM1Adapter=ON" in record["platform"]["configuration"],
         "M1 opt-in build configuration is not recorded")

    image = bind(root, record["image"], "M1 image")
    need(len(image) > 0, "empty M1 image")
    adapter = bind(root, record["implementation"]["root_adapter"], "root adapter")
    wiring = bind(root, record["implementation"]["root_wiring"], "root wiring")
    root_main = bind(root, record["implementation"]["root_main"], "root main")
    client = bind(root, record["implementation"]["taskd_client"], "taskd adapter client")
    child = bind(root, record["implementation"]["child"], "inert child")
    protocol = bind(root, record["implementation"]["protocol"], "M1 protocol")
    cmake = bind(root, record["implementation"]["build_gate"], "M1 build gate")
    runtime = bind(root, record["runtime_evidence"], "M1 runtime log")

    need(record["image"]["path"].startswith("build-root-construct-m1/"),
         "M1 must bind its separate opt-in image")
    for required in (
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST",
        "SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED",
        "SELINOS_ROOT_CONSTRUCT_M1_REJECTED",
        "SELINOS_ROOT_CONSTRUCT_M1_SLOT 1u",
        "SELINOS_ROOT_CONSTRUCT_M1_GENERATION 1u",
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST_WORDS 3u",
        "SELINOS_ROOT_CONSTRUCT_M1_REPLY_WORDS 3u",
        "SELINOS_ROOT_CONSTRUCT_M1_TASKD_REQUEST_ENDPOINT_SLOT 8u",
    ):
        need(required in protocol, f"missing closed M1 protocol control: {required}")
    for excluded in ("pointer", "image", "stack", "register", "TLS", "clone",
                     "scheduler", "credential", "VFS", "device field"):
        need(excluded in protocol, f"protocol does not document exclusion: {excluded}")

    for required in (
        "SeLinRootConstructM1Adapter",
        "SELINOS_ROOT_CONSTRUCT_M1_ADAPTER",
        "DEFAULT\n    OFF",
        "selinos-taskd-root-construct-m1",
        "selinos-root-construct-m1-inert-child",
        "if(SeLinRootConstructM1Adapter)",
        "src/root_construct_m1.c",
    ):
        need(required in cmake, f"missing opt-in build gate control: {required}")

    for required in (
        "SELINOS_ROOT_CONSTRUCT_M1_FREE",
        "SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTING",
        "SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED",
        "SELINOS_ROOT_CONSTRUCT_M1_STATE_REJECTED",
        "root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_FREE;",
        "root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTING;",
        "root_construct_m1_state = SELINOS_ROOT_CONSTRUCT_M1_ROOT_CONSTRUCTED;",
        "sel4utils_configure_process(&child",
        "sel4utils_spawn_process_v(&child, root_construct_m1_vka,",
        "root_construct_m1_vspace, 1, child_argv, 0)",
        "SELINOS_ROOT_CONSTRUCT_M1_REJECTED",
        "malformed or duplicate request rejected; no allocation",
        "one fixed child constructed and spawned suspended; no cap delivered",
    ):
        need(required in adapter, f"missing root M1 adapter control: {required}")
    need(adapter.count("sel4utils_configure_process") == 2,
         "root M1 adapter must configure only its taskd client and one fixed child")
    need(adapter.count("sel4utils_copy_cap_to_process") == 1,
         "M1 may delegate only one taskd endpoint send cap")
    for forbidden in (
        "seL4_SetCap", "seL4_TCB_", "seL4_Untyped_Retype", "seL4_IRQControl",
        "seL4_X86_IOPort", "seL4_X86_IOSpace", "SELINOS_LINUX_CLONE",
        "vka_free", "vspace_free", "sel4utils_destroy_process",
    ):
        need(forbidden not in adapter, f"forbidden M1 root adapter control: {forbidden}")

    loop = source_slice(adapter, "void selinos_root_construct_m1_dispatch_loop(void)", "\n}")
    need("for (;;)" in loop and "seL4_Recv(root_construct_m1_endpoint, &badge)" in loop,
         "M1 must block only on the dedicated construction endpoint")
    for required in (
        "badge == 0u", "seL4_MessageInfo_get_label(message) == 0u",
        "seL4_MessageInfo_get_extraCaps(message) == 0u",
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST_WORDS",
        "SELINOS_ROOT_CONSTRUCT_M1_REQUEST",
        "SELINOS_ROOT_CONSTRUCT_M1_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M1_GENERATION",
        "root_construct_m1_state != SELINOS_ROOT_CONSTRUCT_M1_FREE",
    ):
        need(required in loop, f"missing strict receive validation: {required}")

    for required in (
        "selinos_root_construct_m1_start_bundle(vka, vspace)",
        "CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER",
        "selinos_root_construct_m1_dispatch_loop();",
        "selinos_root_dispatch_m0_once()",
    ):
        need(required in wiring + root_main, f"missing M1/M0 branch control: {required}")
    need("#if CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER" in root_main,
         "root main lacks opt-in construction branch")
    need(root_main.index("selinos_root_construct_m1_dispatch_loop();") <
         root_main.index("root task entering idle/yield loop"),
         "M1 dispatch must precede any idle loop")

    for required in (
        "seL4_Call(", "SELINOS_ROOT_CONSTRUCT_M1_TASKD_REQUEST_ENDPOINT_SLOT",
        "SELINOS_ROOT_CONSTRUCT_M1_CONSTRUCTED",
        "SELINOS_ROOT_CONSTRUCT_M1_REJECTED",
        "fixed construction and duplicate refusal passed; no child cap received",
    ):
        need(required in client, f"missing taskd client control: {required}")
    for forbidden in (
        "vka_", "vspace_", "sel4utils_configure_process", "seL4_SetCap",
        "seL4_TCB_", "seL4_IRQControl", "seL4_X86_IOPort", "seL4_X86_IOSpace",
        "SELINOS_LINUX_",
    ):
        need(forbidden not in client, f"forbidden taskd client authority/control: {forbidden}")
    need("unexpected execution" in child, "inert child lacks unexpected-execution sentinel")
    for forbidden in ("vka_", "vspace_", "seL4_TCB_", "SELINOS_LINUX_"):
        need(forbidden not in child, f"forbidden inert-child authority/control: {forbidden}")

    markers = record["runtime_evidence"]["required_markers"]
    for marker in markers:
        need(marker in runtime, f"missing required runtime marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in runtime, f"forbidden runtime marker: {marker}")
    positions = [runtime.index(marker) for marker in markers]
    need(positions == sorted(positions), "M1 runtime markers are not ordered")

    non_claims = " ".join(record["not_claimed"])
    for excluded in ("general task construction", "child capability delivery", "Linux clone", "dpkg or apt"):
        need(excluded in non_claims, f"missing M1 non-claim: {excluded}")

    print("SeLinOS root-held fixed-child construction adapter M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
