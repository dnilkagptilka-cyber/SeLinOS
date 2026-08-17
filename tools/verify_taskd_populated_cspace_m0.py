#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / "tests/artifacts/selinos_taskd_populated_cspace_m0.verification.json").read_text())
    require(record["schema"] == 1, "schema")
    require(record["profile"]["cmake_option"] == "SeLinTaskdPopulatedCspaceProbe=ON", "default-OFF gate")
    for group in ("images", "implementation"):
        for name, item in record[group].items():
            path = root / item["path"]
            require(path.is_file() and digest(path) == item["sha256"], f"{group}:{name} SHA-256")
    runtime = root / record["runtime"]["path"]
    require(runtime.is_file() and digest(runtime) == record["runtime"]["sha256"], "runtime SHA-256")
    transcript = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime"]["required_markers"]:
        require(marker in transcript, f"runtime marker: {marker}")

    root_source = (root / record["implementation"]["root"]["path"]).read_text()
    cmake = (root / record["implementation"]["cmake"]["path"]).read_text()
    protocol = (root / record["implementation"]["protocol"]["path"]).read_text()
    server = (root / record["implementation"]["server"]["path"]).read_text()
    probe = (root / record["implementation"]["probe"]["path"]).read_text()
    gate = (root / record["implementation"]["gate"]["path"]).read_text()
    for marker in ("SeLinTaskdPopulatedCspaceProbe", "SELINOS_TASKD_POPULATED_CSPACE_PROBE", "selinos-taskd-populated-cspace-m0"):
        require(marker in cmake, f"CMake marker: {marker}")
    for marker in ("SELINOS_TASKD_POPULATED_CSPACE_M0_TARGET_NOTIFICATION_SLOT", "SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT_BITS", "SELINOS_TASKD_POPULATED_CSPACE_M0_TCB_SLOT", "SELINOS_TASKD_POPULATED_CSPACE_M0_VSPACE_ROOT_SLOT"):
        require(marker in protocol, f"protocol marker: {marker}")

    start = root_source.index("static bool start_taskd_populated_cspace_m0")
    end = root_source.index("#endif", start)
    bundle = root_source[start:end]
    copy = "seL4_CNode_Copy(owned_cnode.cptr,"
    asid = "seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,"
    configure = "seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,"
    move = "sel4utils_move_cap_to_process(&taskd,"
    require(bundle.count(copy) == 1, "exactly one final CNode population copy")
    require("SELINOS_TASKD_POPULATED_CSPACE_M0_TARGET_NOTIFICATION_SLOT" in bundle and "SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT_BITS" in bundle, "target slot/radix")
    require(asid in bundle and configure in bundle, "ASID/Configure calls")
    require(bundle.index(copy) < bundle.index(asid) < bundle.index(configure) < bundle.index(move), "copy before ASID before Configure before moves")
    require(bundle.count(move) == 3, "exactly three final move-only transfers")
    require(bundle.index("vka_free_object(vka, &rollback_tcb);") < bundle.index(copy), "generation-1 cleanup before population")
    for forbidden in ("seL4_CNode_Mint", "seL4_CNode_Move", "seL4_CNode_Delete", "seL4_CNode_Revoke", "seL4_TCB_WriteRegisters", "seL4_TCB_Resume", "seL4_TCB_SetPriority", "seL4_X86_PDPT_Map", "seL4_X86_PageDirectory_Map", "seL4_X86_PageTable_Map", "seL4_X86_Page_Map", "clone", "fork", "pthread"):
        require(forbidden not in bundle + server + probe, f"forbidden Phase 56 primitive: {forbidden}")
    require("bool queried = false;" in server and "target resources not invoked" in server, "non-invoking status witness")
    require("SELINOS_TASKD_POPULATED_CSPACE_M0_OWNED" in probe and "SELINOS_TASKD_POPULATED_CSPACE_M0_REJECTED" in probe, "probe sequence")
    require("Slot `1` is a notification capability" in gate and "target remains non-executing" in gate, "design boundary")
    print("SeLinOS one-cap populated CSpace configured non-executing task M0 evidence verified.")


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print(f"verification failed: {error}", file=sys.stderr)
    sys.exit(1)
