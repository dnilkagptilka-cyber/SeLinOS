#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Phase 43 x86_64 execute-disable fixed-page evidence."""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def bind(project: Path, binding: dict, label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def function_slice(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise RuntimeError(f"unterminated function: {signature}")


def ordered(source: str, fragments: tuple[str, ...], message: str) -> None:
    positions = [source.index(fragment) for fragment in fragments]
    require(positions == sorted(positions), message)


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (project / "tests/artifacts/selinos_x86_nx_mapping_m0.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected NX evidence schema")
    profile = evidence["profile"]
    require(profile["build_directory"] == "build-wx-nx-probe",
            "wrong NX proof build directory")
    require(profile["cmake_option"] == "SeLinX86NxMappingProbe=ON",
            "wrong NX proof CMake gate")
    require(profile["kernel_commit"] == "1326364bc9135d9445d936ebc01e38a402c1f4c6",
            "wrong pinned kernel revision")
    require(profile["architecture"] == "x86_64/PC99", "wrong NX proof architecture")
    require(profile["test_virtual_address"] == "0x70000000",
            "wrong fixed NX proof virtual address")
    require(profile["extension"] ==
            "local default-OFF x86 execute-disable VM attribute at bit 3",
            "wrong local extension scope")

    for label, item in evidence["images"].items():
        image = bind(project, item, f"NX {label} image")
        require(image.stat().st_size > 0, f"empty NX {label} image")
    sources = {label: text(bind(project, item, label))
               for label, item in evidence["implementation"].items()}
    runtime_path = bind(project, evidence["runtime_evidence"], "NX runtime log")
    runtime = text(runtime_path)

    for label, padding in (("x86_64_vm_attributes", 60),
                           ("x86_32_vm_attributes", 28)):
        bitfield_layout = re.compile(
            rf"padding\s+{padding}\s+field\s+x86ExecuteDisable\s+1\s+"
            r"field\s+x86PATBit\s+1\s+field\s+x86PCDBit\s+1\s+"
            r"field\s+x86PWTBit\s+1"
        )
        require(bitfield_layout.search(sources[label]) is not None,
                f"{label} must place NX at bit 3 and cache selectors at 2/1/0")
    public_types = sources["public_x86_types"]
    for required in (
        "seL4_X86_Default_VMAttributes = 0",
        "seL4_X86_WriteBack = 0",
        "seL4_X86_WriteThrough = 1",
        "seL4_X86_CacheDisabled = 2",
        "seL4_X86_Uncacheable = 3",
        "seL4_X86_WriteCombining = 4",
        "seL4_X86_ExecuteDisable = (1u << 3)",
    ):
        require(required in public_types, f"missing compatible public attribute: {required}")

    leaves = sources["x86_64_leaf_constructors"]
    require(leaves.count("vm_attributes_get_x86ExecuteDisable(vm_attr), /* xd */") == 3,
            "all and only 4 KiB, 2 MiB, and 1 GiB x86_64 leaf constructors must use NX")
    for signature in ("makeUserPTE", "makeUserPDE", "makeUserPDPTE"):
        leaf = function_slice(leaves, signature)
        require("vm_attributes_get_x86ExecuteDisable(vm_attr), /* xd */" in leaf,
                f"{signature} does not propagate execute-disable to leaf xd")
    decoder = sources["common_x86_frame_decoder"]
    decoder_slice = function_slice(decoder, "decodeX86FrameInvocation")
    ordered(decoder_slice, (
        "vmAttr = vmAttributesFromWord(getSyscallArg(2, buffer));",
        "#ifndef CONFIG_ARCH_X86_64",
        "if (vm_attributes_get_x86ExecuteDisable(vmAttr))",
        "current_syscall_error.type = seL4_InvalidArgument;",
        "current_syscall_error.invalidArgumentNumber = 2;",
        "#endif",
    ), "x86 32-bit execute-disable rejection is not ordered before mapping")

    mapper_header = sources["attribute_aware_mapper_header"]
    mapper = sources["attribute_aware_mapper"]
    require("sel4utils_map_page_with_attributes" in mapper_header,
            "attribute-aware mapper lacks public declaration")
    mapper_slice = function_slice(mapper, "int sel4utils_map_page_with_attributes")
    for required in (
        "seL4_ARCH_Page_Map(frame, vspace_root, (seL4_Word)vaddr,",
        "rights, attributes)",
        "while (error == seL4_FailedLookup)",
        "vspace_get_map_obj(seL4_MappingFailedLookupLevel(),",
        "vka_alloc_object(vka, object_description.type,",
        "vspace_map_obj(&object_description, object.cptr, vspace_root,",
    ):
        require(required in mapper_slice, f"mapper lacks expected control: {required}")

    protocol = sources["probe_protocol"]
    for required in (
        "SELINOS_X86_NX_PROBE_VADDR ((seL4_Word)0x70000000u)",
        "SELINOS_X86_NX_PROBE_SUCCESS_ENDPOINT_SLOT ((seL4_CPtr)8u)",
        "SELINOS_X86_NX_PROBE_RET_OPCODE 0xc3u",
        "SELINOS_X86_NX_PROBE_SUCCESS_WORDS 1u",
    ):
        require(required in protocol, f"missing fixed probe protocol value: {required}")

    probe = sources["root_nx_probe"]
    map_helper = function_slice(probe, "static bool map_fixed_test_page")
    for required in (
        "seL4_X86_ExecuteDisable",
        "SELINOS_X86_NX_PROBE_VADDR",
        "sel4utils_map_page_with_attributes",
        "seL4_AllRights",
    ):
        require(required in map_helper, f"NX map helper lacks required control: {required}")
    probe_function = function_slice(probe, "bool selinos_run_x86_nx_mapping_probe")
    for required in (
        "sel4utils_configure_process",
        "sel4utils_spawn_process_v(&child, root_vka, root_vspace,\n                                  1, argv, 0)",
        "vka_cnode_copy(&child_frame_path, &root_frame_path, seL4_AllRights)",
        "seL4_Fault_VMFault",
        "seL4_VMFault_PrefetchFault",
        "seL4_InstructionFault",
        "seL4_X86_Page_Unmap(child_frame_path.capPtr)",
        "seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u))",
        "SELINOS_X86_NX_PROBE_SUCCESS_MAGIC",
        "vspace_unmap_pages(root_vspace, root_mapping, 1u, seL4_PageBits,",
        "(void)seL4_TCB_Suspend(child.thread.tcb.cptr)",
        "vka_cnode_delete(&child_frame_path)",
        "vka_free_object(root_vka, &test_frame)",
        "vka_free_object(root_vka, &endpoint)",
    ):
        require(required in probe_function, f"NX root probe lacks required control: {required}")
    ordered(probe_function, (
        "vka_cnode_copy(&child_frame_path, &root_frame_path, seL4_AllRights)",
        "map_fixed_test_page(&child, root_vka, child_frame_path.capPtr, true)",
        "write_fixed_ret_opcode(root_vspace, &test_frame, &root_mapping)",
        "seL4_TCB_Resume(child.thread.tcb.cptr)",
        "message = seL4_Recv(child.fault_endpoint.cptr, &badge)",
        "vspace_unmap_pages(root_vspace, root_mapping, 1u, seL4_PageBits,",
        "seL4_X86_Page_Unmap(child_frame_path.capPtr)",
        "map_fixed_test_page(&child, root_vka, child_frame_path.capPtr, false)",
        "seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u))",
        "message = seL4_Recv(endpoint.cptr, &badge)",
        "SELINOS_X86_NX_PROBE_SUCCESS_MAGIC",
    ), "NX fault/remap/control sequence is not strictly ordered")
    require(probe_function.count("write_fixed_ret_opcode(") == 1,
            "NX proof must initialize exactly one fixed byte program")
    require(probe_function.count("map_fixed_test_page(&child, root_vka,") == 2,
            "NX proof must comprise one NX mapping and one executable remap")

    child = sources["child_nx_probe"]
    for required in (
        "void (*const target)(void) = (void (*)(void))SELINOS_X86_NX_PROBE_VADDR;",
        "target();",
        "seL4_SetMR(0, SELINOS_X86_NX_PROBE_SUCCESS_MAGIC);",
        "seL4_Send(SELINOS_X86_NX_PROBE_SUCCESS_ENDPOINT_SLOT,",
        "SELINOS_X86_NX_PROBE_SUCCESS_WORDS",
    ):
        require(required in child, f"NX child lacks bounded witness control: {required}")
    require(child.count("target();") == 1, "NX child must make exactly one fixed call")
    for forbidden in ("execve", "dlopen", "PT_LOAD", "ELF", "reloc", "mmap"):
        require(forbidden not in child, f"NX child contains non-probe execution feature: {forbidden}")

    cmake = sources["build_gate"]
    for required in (
        "SeLinX86NxMappingProbe",
        "SELINOS_X86_NX_MAPPING_PROBE",
        "Enable the isolated x86_64 execute-disable versus executable-control mapping proof",
    ):
        require(required in cmake, f"missing NX CMake gate: {required}")
    wiring = sources["root_wiring"]
    require("#if CONFIG_SELINOS_X86_NX_MAPPING_PROBE" in wiring and
            "selinos_run_x86_nx_mapping_probe" in wiring,
            "NX proof is not correctly root-gated")
    design = sources["phase_design_gate"]
    for required in ("Status: verified, bounded M0 proof.", "Explicit non-claims", "package database, `dpkg` or `apt`"):
        require(required in design, f"missing Phase 43 scope boundary: {required}")

    runtime_evidence = evidence["runtime_evidence"]
    for marker in runtime_evidence["required_markers"]:
        require(marker in runtime, f"missing NX runtime marker: {marker}")
    for marker in runtime_evidence["forbidden_markers"]:
        require(marker not in runtime, f"forbidden NX runtime marker: {marker}")
    ordered(runtime, tuple(runtime_evidence["required_markers"]),
            "NX runtime markers are not in fault-to-control order")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("ELF", "Linux kernel", "Linux VM", "LKL", "dpkg or apt"):
        require(phrase in exclusions, f"missing NX non-claim: {phrase}")

    print("SeLinOS x86_64 execute-disable fixed-page M0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
