#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only zero-descriptor virtio notification M9 evidence."""
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


def bind(project: Path, item: dict, label: str) -> Path:
    path = project / item["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def read(path: Path) -> str:
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
    evidence = json.loads((project / "tests/artifacts/selinos_virtio_zero_descriptor_notification_m9.verification.json").read_text())
    require(evidence["schema"] == 1, "unexpected M9 evidence schema")
    profile = evidence["profile"]
    expected = {
        "build_directory": "build-virtio-zero-descriptor-notification-probe",
        "cmake_option": "SeLinRootVirtioBlkZeroDescriptorNotificationProbe=ON",
        "device_identity": "1af4:1042",
        "driver_features": "zero; no feature selector or driver-feature register write",
        "queue_index": 0,
        "maximum_queue_size_required": "non-zero",
        "programmed_queue_size": 1,
        "queue_enable_before": 0,
        "queue_enable_enabled": 1,
        "queue_enable_after_reset": 0,
        "notification_writes": 1,
        "notification_value": 0,
        "final_device_status": 0,
    }
    for key, value in expected.items():
        require(profile[key] == value, f"wrong M9 profile value: {key}")
    require("zero-filled" in profile["layout"] and "no driver descriptor" in profile["layout"],
            "M9 profile lacks zero-layout/no-descriptor boundary")

    for label, item in evidence["images"].items():
        image = bind(project, item, f"M9 {label} image")
        require(image.stat().st_size > 0, f"empty M9 {label} image")
    sources = {label: read(bind(project, item, label))
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M9 disposable disk fixture")
    fixture_bytes = fixture.read_bytes()
    require(len(fixture_bytes) == 8 * 1024 * 1024 and set(fixture_bytes) <= {0},
            "M9 fixture is not unchanged all-zero 8 MiB media")
    runtime = read(bind(project, evidence["runtime_evidence"], "M9 runtime log"))

    cmake = sources["build_gate"]
    for required in (
        "SeLinRootVirtioBlkZeroDescriptorNotificationProbe",
        "SELINOS_ROOT_VIRTIO_BLK_ZERO_DESCRIPTOR_NOTIFICATION_PROBE",
        "Enable root-only virtio zero-descriptor notification/reset proof",
    ):
        require(required in cmake, f"missing M9 CMake gate: {required}")
    wiring = sources["root_wiring"]
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_ZERO_DESCRIPTOR_NOTIFICATION_PROBE",
        "selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset",
        "one zero-descriptor queue-zero notification then reset passed",
        "no IRQ, DMA claim, containment claim or block I/O",
    ):
        require(required in wiring, f"missing M9 root wiring control: {required}")
    require("selinos_domain_manager_start" in sources["root_main"],
            "M9 must execute only through root bootstrap")

    header = sources["pci_header"]
    for required in (
        "struct selinos_qemu_virtio_zero_descriptor_notification_stage",
        "queue_enable_before", "queue_enable_enabled", "queue_enable_after_reset",
        "layout_frame_paddr", "notification_paddr",
        "selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset",
        "write notification value zero exactly once", "not a DMA/I/O claim",
    ):
        require(required in header, f"missing M9 public contract: {required}")

    pci = sources["pci_helper"]
    for required in (
        "VIRTIO_COMMON_QUEUE_NOTIFY_OFF 0x1eu",
        "VIRTIO_SPLIT_DESC_OFFSET   0u",
        "VIRTIO_SPLIT_DRIVER_OFFSET 16u",
        "VIRTIO_SPLIT_DEVICE_OFFSET 24u",
        "VIRTIO_SPLIT_LAYOUT_BYTES  36u",
        "bool selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset",
    ):
        require(required in pci, f"missing M9 source control: {required}")
    stage = function_slice(pci, "bool selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset")
    for required in (
        "vka_alloc_frame(vka, seL4_PageBits, &layout_frame)",
        "stage->layout_frame_paddr = vka_object_paddr(vka, &layout_frame);",
        "for (size_t index = 0u; index < page_size; ++index)",
        "layout[index] = 0u;",
        "stage->descriptor_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DESC_OFFSET;",
        "stage->driver_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DRIVER_OFFSET;",
        "stage->device_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DEVICE_OFFSET;",
        "*queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;",
        "*queue_enable = 1u;",
        "stage->queue_enable_enabled = *queue_enable;",
        "stage->notification_paddr = notify_range->notification_cap_paddr + notification_relative;",
        "vka_alloc_frame_at(vka, seL4_PageBits, notification_page_paddr,",
        "*notification = VIRTIO_COMMON_QUEUE_ZERO;",
        "stage->queue_enable_after_reset = *queue_enable;",
        "vspace_unmap_pages(root_vspace, notification_mapping, 1u, seL4_PageBits,",
        "vka_free_object(vka, &notification_frame);",
        "vka_free_object(vka, &layout_frame);",
        "vka_free_object(vka, &common_frame);",
    ):
        require(required in stage, f"M9 helper lacks required control: {required}")
    ordered(stage, (
        "for (size_t index = 0u; index < page_size; ++index)",
        "*queue_select = VIRTIO_COMMON_QUEUE_ZERO;",
        "*queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;",
        "*queue_desc_lo = (uint32_t)stage->descriptor_paddr;",
        "*queue_driver_lo = (uint32_t)stage->driver_paddr;",
        "*queue_device_lo = (uint32_t)stage->device_paddr;",
        "*queue_enable = 1u;",
        "stage->notification_paddr = notify_range->notification_cap_paddr + notification_relative;",
        "*notification = VIRTIO_COMMON_QUEUE_ZERO;",
    ), "M9 layout/enable/notification sequence is not strictly ordered")
    final_reset = stage.index(
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;\n"
        "    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];"
    )
    notify_write = stage.index("*notification = VIRTIO_COMMON_QUEUE_ZERO;")
    enable_after_reset = stage.index("stage->queue_enable_after_reset = *queue_enable;")
    require(notify_write < final_reset < enable_after_reset,
            "M9 final reset does not follow notification before queue-enable-zero read-back")
    require(stage.count("    *notification = VIRTIO_COMMON_QUEUE_ZERO;") == 1,
            "M9 must write notification value zero exactly once")
    require(stage.count("    *queue_enable = 1u;") == 1,
            "M9 must write queue_enable=1 exactly once")
    require(stage.count("    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;") == 1,
            "M9 must select queue zero exactly once")
    for forbidden in (
        "layout[0u] =", "layout[1u] =", "layout[2u] =", "layout[16u] =", "layout[24u] =",
        "seL4_IRQHandler", "seL4_IRQControl", "seL4_X86_IOSpace", "seL4_X86_IOPort",
        "PCI_COMMAND_MASTER", "vka_alloc_dma", "sel4utils_copy_cap_to_process",
        "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF", "VIRTIO_COMMON_DEVICE_FEATURE_OFF",
        "block request", "*notification = 1u",
    ):
        require(forbidden not in stage, f"forbidden M9 control: {forbidden}")

    design = sources["phase_design_gate"]
    for required in (
        "Status: verified, bounded M9 proof.", "exactly one 16-bit queue-zero notification write",
        "does **not** establish that DMA cannot occur", "Explicit non-claims",
        "DMA behavior or containment", "dpkg` or `apt`",
    ):
        require(required in design, f"missing M9 design boundary: {required}")

    contract = evidence["validated_contract"]
    for field, phrase in (
        ("layout", "zero-filled"), ("notification", "exactly once"),
        ("queue_state", "queue_enable reads 0"), ("teardown", "released"),
        ("authority", "DMA/containment claim"),
    ):
        require(phrase in contract[field], f"missing M9 contract assertion: {field}")
    runtime_evidence = evidence["runtime_evidence"]
    for marker in runtime_evidence["required_markers"]:
        require(marker in runtime, f"missing M9 runtime marker: {marker}")
    for marker in runtime_evidence["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M9 runtime marker: {marker}")
    before = runtime.rsplit("fixture_before=", 1)[1].splitlines()[0]
    after = runtime.rsplit("fixture_after=", 1)[1].splitlines()[0]
    require(before == after == evidence["fixture"]["sha256"],
            "M9 runtime fixture hashes do not bind unchanged media")

    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("usable virtqueue", "DMA behavior", "block request", "persistent VFS",
                   "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M9 non-claim: {phrase}")
    print("SeLinOS virtio-blk root-only zero-descriptor notification/reset M9 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
