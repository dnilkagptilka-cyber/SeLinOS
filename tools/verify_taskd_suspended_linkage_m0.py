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
        (root / "tests/artifacts/selinos_taskd_suspended_linkage_m0.verification.json").read_text()
    )
    require(record["schema"] == 1, "schema")
    require(record["profile"]["cmake_option"] ==
            "SeLinTaskdSuspendedLinkageProbe=ON", "default-OFF gate")

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

    for marker in ("SeLinTaskdSuspendedLinkageProbe",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_PROBE",
                   "selinos-taskd-suspended-linkage-m0"):
        require(marker in cmake, f"CMake marker: {marker}")
    for marker in ("SELINOS_TASKD_SUSPENDED_LINKAGE_M0_OWNERSHIP_QUERY",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_OWNED",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_REJECTED",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_ROLLBACK_GENERATION",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_OWNED_GENERATION",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_TCB_SLOT",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_CNODE_SLOT",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_VSPACE_ROOT_SLOT"):
        require(marker in protocol, f"protocol marker: {marker}")

    start = root_source.index("static bool start_taskd_suspended_linkage_m0")
    end = root_source.index("#endif", start)
    bundle = root_source[start:end]
    for marker in ("vka_alloc_tcb(vka, &rollback_tcb)",
                   "vka_alloc_vspace_root(vka, &rollback_vspace_root)",
                   "vka_free_object(vka, &rollback_vspace_root);",
                   "vka_free_object(vka, &rollback_cnode);",
                   "vka_free_object(vka, &rollback_tcb);",
                   "vka_alloc_tcb(vka, &owned_tcb)",
                   "vka_alloc_vspace_root(vka, &owned_vspace_root)"):
        require(marker in bundle, f"rollback/allocation marker: {marker}")
    require(bundle.index("vka_free_object(vka, &rollback_tcb);") <
            bundle.index("vka_alloc_tcb(vka, &owned_tcb)"),
            "generation-1 cleanup before generation-2 allocation")

    asid = "seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,"
    configure = "seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,"
    move = "sel4utils_move_cap_to_process(&taskd,"
    require(asid in bundle and configure in bundle, "ASID assignment and TCB Configure")
    require(bundle.index(asid) < bundle.index(configure) < bundle.index(move),
            "ASID assignment before Configure before cap moves")
    require(bundle.count(move) == 3, "exactly three final move-only transfers")
    for marker in ("SELINOS_TASKD_SUSPENDED_LINKAGE_M0_TCB_SLOT",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_CNODE_SLOT",
                   "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_VSPACE_ROOT_SLOT"):
        require(marker in bundle, f"fixed move destination: {marker}")
    for forbidden_copy in ("sel4utils_copy_cap_to_process(&taskd, vka, owned_tcb.cptr)",
                           "sel4utils_copy_cap_to_process(&taskd, vka, owned_cnode.cptr)",
                           "sel4utils_copy_cap_to_process(&taskd, vka, owned_vspace_root.cptr)"):
        require(forbidden_copy not in bundle, f"final cap must move, not copy: {forbidden_copy}")

    forbidden = ("seL4_TCB_WriteRegisters", "seL4_TCB_Resume", "seL4_TCB_SetPriority",
                 "seL4_TCB_SetSchedParams", "seL4_TCB_SetIPCBuffer", "seL4_X86_PDPT_Map",
                 "seL4_X86_PageDirectory_Map", "seL4_X86_PageTable_Map", "seL4_X86_Page_Map",
                 "seL4_CNode_Copy", "seL4_CNode_Move", "seL4_CNode_Mint", "clone",
                 "fork", "pthread")
    for marker in forbidden:
        require(marker not in bundle + server + probe,
                f"forbidden Phase 55 primitive: {marker}")

    require("bool queried = false;" in server and
            "if (!exact_query || queried)" in server and
            "configured TCB not invoked" in server,
            "single-use non-invoking taskd witness")
    require("SELINOS_TASKD_SUSPENDED_LINKAGE_M0_OWNED" in probe and
            "SELINOS_TASKD_SUSPENDED_LINKAGE_M0_REJECTED" in probe,
            "probe owned/rejected sequence")
    require("A configured TCB is not an executing task." in gate and
            "`seL4_TCB_WriteRegisters`" in gate and
            "`seL4_TCB_Resume`" in gate,
            "design gate boundary")
    print("SeLinOS configured but non-executing task linkage M0 evidence verified.")


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print(f"verification failed: {error}", file=sys.stderr)
    sys.exit(1)
