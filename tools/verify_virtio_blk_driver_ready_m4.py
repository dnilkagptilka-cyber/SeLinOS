#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk driver-ready/reset M4 evidence."""
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
        (project / "tests/artifacts/selinos_virtio_blk_driver_ready_m4.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected M4 driver-ready evidence schema")
    profile = evidence["profile"]
    require(profile["build_directory"] == "build-virtio-driver-ready-probe",
            "wrong M4 driver-ready build directory")
    require(profile["cmake_option"] == "SeLinRootVirtioBlkDriverReadyProbe=ON",
            "wrong M4 driver-ready CMake gate")
    require(profile["device_identity"] == "1af4:1042", "wrong virtio fixture identity")
    require(profile["common_configuration_bar"] ==
            "QEMU fixture BAR4: 64-bit prefetchable memory", "wrong common BAR fixture")
    require(profile["iommu_platform"] is False and profile["ats"] is False,
            "M4 must not claim IOMMU/ATS fixture containment")
    require(profile["driver_features"] ==
            "zero; no feature selector or driver-feature register write",
            "M4 must keep zero driver features")
    require(profile["driver_ok_readback"] is True, "M4 must bind DRIVER_OK read-back")
    require(profile["final_device_status"] == 0, "M4 must record final reset status zero")

    image = bind(project, evidence["image"], "M4 driver-ready image")
    require(len(image) > 0, "empty M4 driver-ready image")
    sources = {label: bind(project, item, label)
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M4 disposable disk fixture")
    require(len(fixture) == 8 * 1024 * 1024, "M4 fixture must remain an 8 MiB raw file")
    require(set(fixture) <= {"\x00"}, "M4 fixture changed despite no block request claim")
    runtime = bind(project, evidence["runtime_evidence"], "M4 runtime log")

    pci = sources["pci_helper"]
    header = sources["pci_header"]
    wiring = sources["root_wiring"]
    cmake = sources["build_gate"]
    root_main = sources["root_main"]

    for required in (
        "SeLinRootVirtioBlkDriverReadyProbe",
        "SELINOS_ROOT_VIRTIO_BLK_DRIVER_READY_PROBE",
        "Enable root-only zero-feature virtio DRIVER_OK/reset proof",
    ):
        require(required in cmake, f"missing M4 build-gate control: {required}")
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_DRIVER_READY_PROBE",
        "selinos_pci_inspect_qemu_virtio_blk_common_capability",
        "selinos_pci_validate_qemu_virtio_blk_common_range",
        "selinos_pci_stage_qemu_virtio_blk_driver_ready_reset",
        "root-only zero-feature DRIVER_OK accepted then reset to zero",
        "no queue, IRQ, DMA, bus mastering or block I/O",
    ):
        require(required in wiring, f"missing root M4 wiring control: {required}")
    require("selinos_domain_manager_start" in root_main,
            "M4 must execute only through root bootstrap")

    for required in (
        "struct selinos_qemu_virtio_driver_ready_stage",
        "status_after_driver_ok", "status_after_reset",
        "selinos_pci_stage_qemu_virtio_blk_driver_ready_reset",
        "queue", "DMA", "block-I/O state",
    ):
        require(required in header, f"missing M4 public contract control: {required}")
    for required in (
        "VIRTIO_STATUS_DRIVER_OK          0x04u",
        "selinos_pci_stage_qemu_virtio_blk_driver_ready_reset",
        "vka_alloc_frame_at", "vspace_reserve_range_aligned",
        "vspace_map_pages_at_vaddr", "vspace_unmap_pages", "VSPACE_PRESERVE",
        "vka_free_object", "status_after_driver_ok", "status_after_reset",
    ):
        require(required in pci, f"missing M4 status-stage control: {required}")
    stage = function_slice(pci, "bool selinos_pci_stage_qemu_virtio_blk_driver_ready_reset")
    sequence = [
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;",
        "VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;",
        "stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
    ]
    positions = [stage.index(fragment) for fragment in sequence]
    require(positions == sorted(positions), "M4 status sequence source order is invalid")
    require(stage.count("VIRTIO_STATUS_DRIVER_OK") == 1,
            "M4 must contain exactly one DRIVER_OK stage")
    require(stage.count("common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =") == 7,
            "M4 must contain six normal stage/reset writes plus one fail-closed reset")
    for forbidden in (
        "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF", "VIRTIO_COMMON_DEVICE_FEATURE_OFF",
        "queue_select", "queue_size", "queue_enable", "queue_notify", "virtqueue",
        "seL4_IRQHandler", "seL4_IRQControl", "seL4_X86_IOSpace", "seL4_X86_IOPort",
        "PCI_COMMAND_MASTER", "block request", "vka_alloc_dma", "sel4utils_copy_cap_to_process",
    ):
        require(forbidden not in stage, f"forbidden M4 driver/I/O control: {forbidden}")

    contract = evidence["validated_contract"]
    require(contract["status_sequence"] ==
            "0 -> ACKNOWLEDGE -> ACKNOWLEDGE|DRIVER -> ACKNOWLEDGE|DRIVER|FEATURES_OK -> ACKNOWLEDGE|DRIVER|FEATURES_OK|DRIVER_OK -> 0",
            "wrong M4 status-sequence contract")
    for field, phrase in (("readback", "DRIVER_OK"), ("mapping", "released"),
                          ("zero_feature_policy", "no device-feature selector"),
                          ("authority", "no cap copy")):
        require(phrase in contract[field], f"missing M4 contract assertion: {field}")

    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing M4 runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M4 runtime marker: {marker}")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("virtio driver", "DMA", "block request", "persistent VFS",
                   "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M4 non-claim: {phrase}")

    print("SeLinOS virtio-blk root-only zero-feature driver-ready/reset M4 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
