#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk zero-feature status-stage M3 evidence."""
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


def bind(project: Path, binding: dict, label: str) -> str:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
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


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (project / "tests/artifacts/selinos_virtio_blk_feature_stage_m3.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected M3 feature-stage evidence schema")
    profile = evidence["profile"]
    require(profile["build_directory"] == "build-virtio-feature-stage-probe",
            "wrong M3 feature-stage build directory")
    require(profile["cmake_option"] == "SeLinRootVirtioBlkFeatureStageProbe=ON",
            "wrong M3 feature-stage CMake gate")
    require(profile["device_identity"] == "1af4:1042", "wrong virtio fixture identity")
    require(profile["common_configuration_bar"] ==
            "QEMU fixture BAR4: 64-bit prefetchable memory", "wrong common BAR fixture")
    require(profile["iommu_platform"] is False and profile["ats"] is False,
            "M3 must not claim IOMMU/ATS fixture containment")
    require(profile["driver_features"] ==
            "zero; no feature selector or driver-feature register write",
            "M3 must keep zero driver features")
    require(profile["final_device_status"] == 0, "M3 must record final reset status zero")

    image = bind(project, evidence["image"], "M3 feature-stage image")
    require(len(image) > 0, "empty M3 feature-stage image")
    sources = {label: bind(project, item, label)
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M3 disposable disk fixture")
    require(len(fixture) == 8 * 1024 * 1024, "M3 fixture must remain an 8 MiB raw file")
    require(set(fixture) <= {"\x00"}, "M3 fixture changed despite no block request claim")
    runtime = bind(project, evidence["runtime_evidence"], "M3 runtime log")

    pci = sources["pci_helper"]
    header = sources["pci_header"]
    wiring = sources["root_wiring"]
    cmake = sources["build_gate"]
    root_main = sources["root_main"]

    for required in (
        "SeLinRootVirtioBlkFeatureStageProbe",
        "SELINOS_ROOT_VIRTIO_BLK_FEATURE_STAGE_PROBE",
        "Enable root-only zero-feature virtio status staging and reset proof",
    ):
        require(required in cmake, f"missing M3 build-gate control: {required}")
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_FEATURE_STAGE_PROBE",
        "selinos_pci_inspect_qemu_virtio_blk_common_capability",
        "selinos_pci_validate_qemu_virtio_blk_common_range",
        "selinos_pci_stage_qemu_virtio_blk_zero_features",
        "root-only zero-feature FEATURES_OK accepted then reset to zero",
        "no DRIVER_OK, queue, IRQ, DMA, bus mastering or block I/O",
    ):
        require(required in wiring, f"missing root M3 wiring control: {required}")
    require("selinos_domain_manager_start" in root_main,
            "M3 must execute only through root bootstrap")

    for required in (
        "struct selinos_qemu_virtio_feature_stage",
        "status_after_features_ok",
        "status_after_reset",
        "selinos_pci_stage_qemu_virtio_blk_zero_features",
        "DRIVER_OK", "queue", "DMA", "block I/O",
    ):
        require(required in header, f"missing M3 public contract control: {required}")
    for required in (
        "VIRTIO_COMMON_DEVICE_STATUS_OFF 0x14u",
        "VIRTIO_STATUS_ACKNOWLEDGE        0x01u",
        "VIRTIO_STATUS_DRIVER             0x02u",
        "VIRTIO_STATUS_FEATURES_OK        0x08u",
        "selinos_pci_stage_qemu_virtio_blk_zero_features",
        "vka_alloc_frame_at",
        "vspace_reserve_range_aligned",
        "vspace_map_pages_at_vaddr",
        "vspace_unmap_pages",
        "VSPACE_PRESERVE",
        "vka_free_object",
        "status_after_features_ok",
        "status_after_reset",
    ):
        require(required in pci, f"missing M3 status-stage control: {required}")
    stage_source = function_slice(pci, "bool selinos_pci_stage_qemu_virtio_blk_zero_features")
    sequence = [
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;",
        "VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;",
        "VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;",
        "stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
    ]
    positions = [stage_source.index(fragment) for fragment in sequence]
    require(positions == sorted(positions), "M3 status sequence source order is invalid")
    for forbidden in (
        "VIRTIO_STATUS_DRIVER_OK", "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF",
        "VIRTIO_COMMON_DEVICE_FEATURE_OFF", "queue_select", "queue_size",
        "queue_enable", "queue_notify", "virtqueue", "seL4_IRQHandler",
        "seL4_IRQControl", "seL4_X86_IOSpace", "seL4_X86_IOPort",
        "PCI_COMMAND_MASTER", "block request", "vka_alloc_dma", "sel4utils_copy_cap_to_process",
    ):
        require(forbidden not in stage_source, f"forbidden M3 driver/I/O control: {forbidden}")
    require(stage_source.count("common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =") == 6,
            "M3 must contain five normal stage/reset writes plus one fail-closed reset")

    contract = evidence["validated_contract"]
    require(contract["status_sequence"] ==
            "0 -> ACKNOWLEDGE -> ACKNOWLEDGE|DRIVER -> ACKNOWLEDGE|DRIVER|FEATURES_OK -> 0",
            "wrong M3 status-sequence contract")
    for field, phrase in (("acceptance", "FEATURES_OK"), ("mapping", "released"),
                          ("zero_feature_policy", "no device-feature selector"),
                          ("authority", "no cap copy")):
        require(phrase in contract[field], f"missing M3 contract assertion: {field}")

    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing M3 runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M3 runtime marker: {marker}")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("DRIVER_OK", "virtio driver", "DMA", "block request",
                   "persistent VFS", "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M3 non-claim: {phrase}")

    print("SeLinOS virtio-blk root-only zero-feature status-stage M3 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
