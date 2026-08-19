#!/usr/bin/env python3
"""Independent verifier for Phase 43 post-NXE raw-FSR revalidation M1."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "tests/artifacts/selinos_x86_nx_raw_fsr_revalidation_m1.verification.json"


def fail(message: str) -> None:
    print(f"verification failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def function_slice(source: str, signature: str) -> str:
    start = source.find(signature)
    require(start >= 0, f"missing function {signature}")
    brace = source.find("{", start)
    require(brace >= 0, f"missing body for {signature}")
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    fail(f"unterminated function {signature}")


def main() -> None:
    evidence = json.loads(EVIDENCE.read_text())
    require(evidence["gate"] == "Phase 43 x86 NX raw-FSR revalidation M1", "wrong gate")
    require(evidence["status"].startswith("verified;"), "M1 evidence is not verified")
    require(evidence["observed_branch"] == "execute_disable_protection_fsr_0x15",
            "wrong Phase 43 revalidation branch")

    checks: list[tuple[Path, str]] = []
    for item in evidence["images"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    checks.append((ROOT / evidence["runtime"]["path"], evidence["runtime"]["sha256"]))
    for item in evidence["implementation"].values():
        checks.append((ROOT / item["path"], item["sha256"]))
    for path, expected in checks:
        require(path.is_file(), f"missing SHA-bound file {path.relative_to(ROOT)}")
        require(sha256(path) == expected, f"SHA-256 mismatch for {path.relative_to(ROOT)}")

    cmake = (ROOT / evidence["implementation"]["cmake"]["path"]).read_text()
    option_at = cmake.find("SeLinX86NxRawFsrRevalidationProbe")
    require(option_at >= 0, "missing raw-FSR CMake option")
    option = cmake[option_at:cmake.find(")", option_at) + 1]
    require("SELINOS_X86_NX_RAW_FSR_REVALIDATION_PROBE" in option,
            "missing raw-FSR config macro")
    require("DEFAULT\n    OFF" in option, "raw-FSR profile must default OFF")
    require("SeLinX86NxMappingProbe=OFF" == evidence["profile"]["disabled_peer_option"],
            "historical NX profile must be disabled")

    head = (ROOT / evidence["implementation"]["x86_head"]["path"]).read_text()
    for token in ("BEGIN_FUNC(nxe_check)", "call nxe_check", "orl $0x900, %eax"):
        require(token in head, f"missing NXE boot control {token}")

    protocol = (ROOT / evidence["implementation"]["protocol"]["path"]).read_text()
    for token in (
        "SELINOS_X86_NX_PROBE_VADDR ((seL4_Word)0x70000000u)",
        "SELINOS_X86_NX_PROBE_RET_OPCODE 0xc3u",
        "SELINOS_X86_NX_PROBE_EXECUTE_DISABLE_FSR 0x15u",
    ):
        require(token in protocol, f"missing protocol token {token}")

    probe = (ROOT / evidence["implementation"]["root_probe"]["path"]).read_text()
    body = function_slice(probe, "bool selinos_run_x86_nx_mapping_probe")
    for token in (
        "seL4_VMFault_FSR",
        "SELINOS_X86_NX_PROBE_EXECUTE_DISABLE_FSR",
        "seL4_InstructionFault",
        "seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u))",
        "sel4utils_destroy_process(&child, root_vka)",
        "(void)seL4_TCB_Suspend(child.thread.tcb.cptr)",
    ):
        require(token in body, f"missing Phase 43 revalidation control {token}")
    require(body.count("seL4_TCB_Resume(child.thread.tcb.cptr)") == 1,
            "must resume the child exactly once")
    require(body.count("seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u))") == 1,
            "must reply exactly once after executable remap")
    require(body.find("seL4_VMFault_FSR") < body.find("seL4_X86_Page_Unmap(child_frame_path.capPtr)"),
            "raw FSR must be checked before NX mapping is removed")
    require(body.find("(void)seL4_TCB_Suspend(child.thread.tcb.cptr)") <
            body.find("sel4utils_destroy_process(&child, root_vka)"),
            "child must be suspended before destruction")

    runtime = (ROOT / evidence["runtime"]["path"]).read_text(errors="replace")
    for marker in evidence["runtime"]["required_markers"]:
        require(marker in runtime, f"missing QEMU marker: {marker}")
    for marker in evidence["runtime"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden QEMU marker: {marker}")
    require(runtime.find("raw FSR 0x15") < runtime.find("executable control returned fixed witness"),
            "runtime order does not prove NX fault before executable control")

    print("verification passed: Phase 43 x86 NX post-NXE raw-FSR revalidation M1")


if __name__ == "__main__":
    main()
