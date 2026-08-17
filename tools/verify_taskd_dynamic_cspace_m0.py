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
        (root / "tests/artifacts/selinos_taskd_dynamic_cspace_m0.verification.json").read_text()
    )
    require(record["schema"] == 1, "schema")
    require(record["profile"]["cmake_option"] ==
            "SeLinTaskdDynamicCspaceRollbackProbe=ON", "default-OFF gate")

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

    for marker in ("SeLinTaskdDynamicCspaceRollbackProbe",
                   "SELINOS_TASKD_DYNAMIC_CSPACE_ROLLBACK_PROBE",
                   "selinos-taskd-dynamic-cspace-m0"):
        require(marker in cmake, f"CMake marker: {marker}")
    for marker in ("SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNERSHIP_QUERY",
                   "SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNED",
                   "SELINOS_TASKD_DYNAMIC_CSPACE_M0_REJECTED",
                   "SELINOS_TASKD_DYNAMIC_CSPACE_M0_ROLLBACK_GENERATION",
                   "SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNED_GENERATION"):
        require(marker in protocol, f"protocol marker: {marker}")

    start = root_source.index("static bool start_taskd_dynamic_cspace_m0_bundle")
    end = root_source.index("#endif", start)
    bundle = root_source[start:end]
    allocate = "vka_alloc_cnode_object(vka, SELINOS_TASKD_DYNAMIC_CSPACE_M0_SLOT_BITS,"
    require(bundle.count(allocate) == 2, "exactly two bounded CNode allocation calls")
    free_marker = "vka_free_object(vka, &rollback_cnode);"
    move_marker = "sel4utils_move_cap_to_process(&taskd, root_owned_path, vka)"
    require(free_marker in bundle and move_marker in bundle,
            "rollback and move markers")
    require(bundle.index(free_marker) < bundle.index(move_marker),
            "generation-1 rollback must precede final CNode move")
    require("SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNED_CNODE_SLOT" in bundle,
            "fixed taskd destination slot")
    require("sel4utils_copy_cap_to_process(&taskd, vka, owned_cnode.cptr)" not in bundle,
            "final CNode must move, not copy")

    for forbidden in ("seL4_TCB_Configure", "seL4_TCB_SetSpace", "seL4_TCB_WriteRegisters",
                      "seL4_TCB_Resume", "seL4_CNode_Copy", "seL4_CNode_Move",
                      "seL4_CNode_Mint", "clone", "fork", "pthread"):
        require(forbidden not in bundle + server + probe,
                f"forbidden Phase 52 primitive: {forbidden}")

    require("bool queried = false;" in server and
            "if (!exact_query || queried)" in server and
            "final CNode not invoked" in server,
            "single-use non-invoking taskd witness")
    require("SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNED" in probe and
            "SELINOS_TASKD_DYNAMIC_CSPACE_M0_REJECTED" in probe,
            "probe owned/rejected sequence")
    require("pre-delegation policy-reject" in gate and
            "final CNode is an inert capability-table object" in gate,
            "design gate contract")
    print("SeLinOS taskd-owned dynamic CNode rollback and ownership M0 evidence verified.")


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print(f"verification failed: {error}", file=sys.stderr)
    sys.exit(1)
