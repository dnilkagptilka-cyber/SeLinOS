#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk queue-enable/read-back/reset M7 evidence."""
from __future__ import annotations

import hashlib
import json
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


def read_text(path: Path) -> str:
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
    offsets = [source.index(fragment) for fragment in fragments]
    require(offsets == sorted(offsets), message)


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (project / "tests/artifacts/selinos_virtio_queue_enable_m7.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected M7 evidence schema")
    profile = evidence["profile"]
    expected_profile = {
        "build_directory": "build-virtio-queue-enable-probe",
        "cmake_option": "SeLinRootVirtioBlkQueueEnableProbe=ON",
        "device_identity": "1af4:1042",
        "common_configuration_bar": "QEMU fixture BAR4: 64-bit prefetchable memory",
        "driver_features": "zero; no feature selector or driver-feature register write",
        "queue_index": 0,
        "maximum_queue_size_required": "non-zero",
        "programmed_queue_size": 1,
        "queue_enable_before": 0,
        "queue_enable_enabled": 1,
        "queue_enable_after_reset": 0,
        "final_device_status": 0,
    }
    for field, value in expected_profile.items():
        require(profile[field] == value, f"wrong M7 profile value: {field}")
    require(profile["iommu_platform"] is False and profile["ats"] is False,
            "M7 must not claim IOMMU/ATS containment")
    require("unpublished" in profile["queue_memory"],
            "M7 queue memory must remain unpublished")

    for label, item in evidence["images"].items():
        image = bind(project, item, f"M7 {label} image")
        require(image.stat().st_size > 0, f"empty M7 {label} image")
    sources = {label: read_text(bind(project, item, label))
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M7 disposable disk fixture")
    fixture_bytes = fixture.read_bytes()
    require(len(fixture_bytes) == 8 * 1024 * 1024,
            "M7 fixture must remain an 8 MiB raw file")
    require(set(fixture_bytes) <= {0},
            "M7 fixture changed despite no-notification/no-I/O proof")
    runtime = read_text(bind(project, evidence["runtime_evidence"], "M7 runtime log"))

    cmake = sources["build_gate"]
    for required in (
        "SeLinRootVirtioBlkQueueEnableProbe",
        "SELINOS_ROOT_VIRTIO_BLK_QUEUE_ENABLE_PROBE",
        "Enable root-only virtio queue-enable/read-back/reset proof",
    ):
        require(required in cmake, f"missing M7 CMake gate: {required}")

    wiring = sources["root_wiring"]
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_QUEUE_ENABLE_PROBE",
        "selinos_pci_inspect_qemu_virtio_blk_common_capability",
        "selinos_pci_validate_qemu_virtio_blk_common_range",
        "selinos_pci_stage_qemu_virtio_blk_queue_enable_reset",
        "queue_enable one read-back then reset-to-zero passed",
        "no notify, IRQ, PCI-command write, block I/O or DMA/containment claim",
    ):
        require(required in wiring, f"missing M7 root wiring control: {required}")
    require("selinos_domain_manager_start" in sources["root_main"],
            "M7 must execute only through root bootstrap")

    header = sources["pci_header"]
    for required in (
        "struct selinos_qemu_virtio_queue_enable_stage",
        "queue_enable_before", "queue_enable_enabled", "queue_enable_after_reset",
        "frame_paddr", "descriptor_paddr", "driver_paddr", "device_paddr",
        "selinos_pci_stage_qemu_virtio_blk_queue_enable_reset",
        "this is not a DMA-behavior or containment claim.",
    ):
        require(required in header, f"missing M7 public contract control: {required}")

    pci = sources["pci_helper"]
    for required in (
        "VIRTIO_COMMON_QUEUE_SELECT_OFF 0x16u",
        "VIRTIO_COMMON_QUEUE_SIZE_OFF   0x18u",
        "VIRTIO_COMMON_QUEUE_ENABLE_OFF    0x1cu",
        "VIRTIO_COMMON_QUEUE_DESC_LO_OFF   0x20u",
        "VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF 0x34u",
        "VIRTIO_COMMON_QUEUE_LAYOUT_SIZE   1u",
        "VIRTIO_SPLIT_DESC_OFFSET   0u",
        "VIRTIO_SPLIT_DRIVER_OFFSET 16u",
        "VIRTIO_SPLIT_DEVICE_OFFSET 24u",
        "VIRTIO_SPLIT_LAYOUT_BYTES  36u",
        "vka_object_paddr",
    ):
        require(required in pci, f"missing M7 source layout control: {required}")

    stage = function_slice(pci, "bool selinos_pci_stage_qemu_virtio_blk_queue_enable_reset")
    for required in (
        "vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &common_frame)",
        "vka_alloc_frame(vka, seL4_PageBits, &layout_frame)",
        "stage->frame_paddr = vka_object_paddr(vka, &layout_frame);",
        "for (size_t index = 0u; index < page_size; ++index)",
        "layout[index] = 0u;",
        "stage->descriptor_paddr = stage->frame_paddr + VIRTIO_SPLIT_DESC_OFFSET;",
        "stage->driver_paddr = stage->frame_paddr + VIRTIO_SPLIT_DRIVER_OFFSET;",
        "stage->device_paddr = stage->frame_paddr + VIRTIO_SPLIT_DEVICE_OFFSET;",
        "stage->queue_enable_before = *queue_enable;",
        "*queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;",
        "*queue_desc_lo = (uint32_t)stage->descriptor_paddr;",
        "*queue_driver_lo = (uint32_t)stage->driver_paddr;",
        "*queue_device_lo = (uint32_t)stage->device_paddr;",
        "*queue_enable = 1u;",
        "stage->queue_enable_enabled = *queue_enable;",
        "stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
        "stage->queue_enable_after_reset = *queue_enable;",
        "stage->queue_enable_after_reset == 0u",
        "vspace_unmap_pages(root_vspace, layout_mapping, 1u, seL4_PageBits,",
        "vspace_free_reservation(root_vspace, layout_reservation);",
        "vka_free_object(vka, &layout_frame);",
        "vka_free_object(vka, &common_frame);",
    ):
        require(required in stage, f"M7 helper lacks required control: {required}")
    ordered(stage, (
        "for (size_t index = 0u; index < page_size; ++index)",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;",
        "*queue_select = VIRTIO_COMMON_QUEUE_ZERO;",
        "stage->queue_enable_before = *queue_enable;",
        "*queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;",
        "*queue_desc_lo = (uint32_t)stage->descriptor_paddr;",
        "*queue_driver_lo = (uint32_t)stage->driver_paddr;",
        "*queue_device_lo = (uint32_t)stage->device_paddr;",
        "*queue_enable = 1u;",
        "stage->queue_enable_enabled = *queue_enable;",
        "stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
        "stage->queue_enable_after_reset = *queue_enable;",
    ), "M7 zero/layout/enable/reset sequence is not strictly ordered")
    require(stage.count("    *queue_enable = 1u;") == 1,
            "M7 must write queue_enable=1 exactly once")
    require("    *queue_enable = 0u;" not in stage,
            "M7 must not write queue_enable=0; reset must establish zero")
    require(stage.count("*queue_select = VIRTIO_COMMON_QUEUE_ZERO;") == 1,
            "M7 must select queue zero exactly once")
    require(stage.count("*queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;") == 1,
            "M7 must write queue size exactly once")
    for forbidden in (
        "queue_notify", "seL4_IRQHandler", "seL4_IRQControl", "seL4_X86_IOSpace",
        "seL4_X86_IOPort", "PCI_COMMAND_MASTER", "vka_alloc_dma",
        "sel4utils_copy_cap_to_process", "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF",
        "VIRTIO_COMMON_DEVICE_FEATURE_OFF", "block request",
    ):
        require(forbidden not in stage, f"forbidden M7 queue/I-O control: {forbidden}")

    design = sources["phase_design_gate"]
    for required in (
        "Status: verified, bounded M7 proof.", "queue_enable=1", "queue_enable=0",
        "Explicit non-claims", "DMA behavior or containment", "dpkg`, or `apt`",
    ):
        require(required in design, f"missing M7 design boundary: {required}")

    contract = evidence["validated_contract"]
    for field, phrase in (
        ("layout", "zero-filled"), ("enable_policy", "exact read-back"),
        ("mapping", "released"), ("authority", "no copied capability"),
    ):
        require(phrase in contract[field], f"missing M7 contract assertion: {field}")
    runtime_evidence = evidence["runtime_evidence"]
    for marker in runtime_evidence["required_markers"]:
        require(marker in runtime, f"missing M7 runtime marker: {marker}")
    for marker in runtime_evidence["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M7 runtime marker: {marker}")
    before = runtime.rsplit("fixture_before=", 1)[1].splitlines()[0]
    after = runtime.rsplit("fixture_after=", 1)[1].splitlines()[0]
    require(before == after == evidence["fixture"]["sha256"],
            "M7 runtime fixture hashes do not bind unchanged media")

    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("usable or safe virtqueue", "DMA behavior or containment",
                   "block request", "persistent VFS", "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M7 non-claim: {phrase}")

    print("SeLinOS virtio-blk root-only queue-enable/read-back/reset M7 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
