#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def digest(path: Path) -> str:
    sha = hashlib.sha256()
    sha.update(path.read_bytes())
    return sha.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    record = json.loads(
        (root / "tests/artifacts/selinos_taskd_inert_bundle_m0.verification.json").read_text()
    )
    require(record["schema"] == 1, "schema")
    require(record["profile"]["cmake_option"] ==
            "SeLinTaskdInertBundleRollbackProbe=ON", "default-OFF gate")

    for group in ("images", "implementation"):
        for name, item in record[group].items():
            path = root / item["path"]
            require(path.is_file() and digest(path) == item["sha256"],
                    f"{group}:{name} SHA-256")

    runtime = root / record["runtime"]["path"]
    require(runtime.is_file() and digest(runtime) == record["runtime"]["sha256"],
            "runtime SHA-256")
    transcript = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime"]["required_markers"]:
        require(marker in transcript, f"runtime marker: {marker}")

    root_source = (root / record["implementation"]["root"]["path"]).read_text()
    cmake = (root / record["implementation"]["cmake"]["path"]).read_text()
    protocol = (root / record["implementation"]["protocol"]["path"]).read_text()
    server = (root / record["implementation"]["server"]["path"]).read_text()
    probe = (root / record["implementation"]["probe"]["path"]).read_text()
    gate = (root / record["implementation"]["gate"]["path"]).read_text()

    for marker in ("SeLinTaskdInertBundleRollbackProbe",
                   "SELINOS_TASKD_INERT_BUNDLE_ROLLBACK_PROBE",
                   "selinos-taskd-inert-bundle-m0"):
        require(marker in cmake, f"CMake marker: {marker}")
    for marker in ("SELINOS_TASKD_INERT_BUNDLE_M0_OWNERSHIP_QUERY",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_OWNED",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_REJECTED",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_ROLLBACK_GENERATION",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_OWNED_GENERATION",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_TCB_SLOT",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_VSPACE_ROOT_SLOT"):
        require(marker in protocol, f"protocol marker: {marker}")

    start = root_source.index("static bool start_taskd_inert_bundle_m0")
    end = root_source.index("#endif", start)
    bundle = root_source[start:end]
    required_generation_one = (
        "vka_alloc_tcb(vka, &rollback_tcb)",
        "vka_alloc_cnode_object(vka, SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT_BITS,",
        "vka_alloc_vspace_root(vka, &rollback_vspace_root)",
        "vka_free_object(vka, &rollback_vspace_root);",
        "vka_free_object(vka, &rollback_cnode);",
        "vka_free_object(vka, &rollback_tcb);",
    )
    for marker in required_generation_one:
        require(marker in bundle, f"generation-1 marker: {marker}")
    require(bundle.index("vka_alloc_vspace_root(vka, &rollback_vspace_root)") <
            bundle.index("vka_free_object(vka, &rollback_vspace_root);") <
            bundle.index("vka_free_object(vka, &rollback_cnode);") <
            bundle.index("vka_free_object(vka, &rollback_tcb);"),
            "generation-1 resources must be freed in reverse allocation order")

    final_move = "sel4utils_move_cap_to_process(&taskd,"
    require(bundle.count(final_move) == 3, "exactly three final move-only transfers")
    require(bundle.index("vka_free_object(vka, &rollback_tcb);") <
            bundle.index("vka_alloc_tcb(vka, &owned_tcb)"),
            "generation-1 cleanup precedes generation-2 allocation")
    for marker in ("SELINOS_TASKD_INERT_BUNDLE_M0_TCB_SLOT",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT",
                   "SELINOS_TASKD_INERT_BUNDLE_M0_VSPACE_ROOT_SLOT"):
        require(marker in bundle, f"fixed move destination: {marker}")
    for forbidden_copy in ("sel4utils_copy_cap_to_process(&taskd, vka, owned_tcb.cptr)",
                           "sel4utils_copy_cap_to_process(&taskd, vka, owned_cnode.cptr)",
                           "sel4utils_copy_cap_to_process(&taskd, vka, owned_vspace_root.cptr)"):
        require(forbidden_copy not in bundle, f"final cap must move, not copy: {forbidden_copy}")

    forbidden = ("seL4_TCB_Configure", "seL4_TCB_SetSpace", "seL4_TCB_WriteRegisters",
                 "seL4_TCB_Resume", "seL4_X86_ASIDPool_Assign", "seL4_X86_PDPT_Map",
                 "seL4_X86_PageDirectory_Map", "seL4_X86_PageTable_Map", "seL4_X86_Page_Map",
                 "seL4_CNode_Copy", "seL4_CNode_Move", "seL4_CNode_Mint", "clone",
                 "fork", "pthread")
    for marker in forbidden:
        require(marker not in bundle + server + probe,
                f"forbidden Phase 54 primitive: {marker}")

    require("bool queried = false;" in server and
            "if (!exact_query || queried)" in server and
            "no final resource invoked" in server,
            "single-use non-invoking taskd witness")
    require("SELINOS_TASKD_INERT_BUNDLE_M0_OWNED" in probe and
            "SELINOS_TASKD_INERT_BUNDLE_M0_REJECTED" in probe,
            "probe owned/rejected sequence")
    require("A resource bundle is **not a task**." in gate and
            "final resources remain mutually unlinked" in gate,
            "design gate boundary")
    print("SeLinOS taskd-owned inert task-resource bundle rollback and ownership M0 evidence verified.")


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print(f"verification failed: {error}", file=sys.stderr)
    sys.exit(1)
