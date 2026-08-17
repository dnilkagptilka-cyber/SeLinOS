#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk queue-zero observation M5 evidence."""
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
        (project / "tests/artifacts/selinos_virtio_blk_queue_zero_m5.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected M5 queue-zero evidence schema")
    profile = evidence["profile"]
    require(profile["build_directory"] == "build-virtio-queue-zero-probe",
            "wrong M5 queue-zero build directory")
    require(profile["cmake_option"] == "SeLinRootVirtioBlkQueueZeroProbe=ON",
            "wrong M5 queue-zero CMake gate")
    require(profile["device_identity"] == "1af4:1042", "wrong virtio fixture identity")
    require(profile["common_configuration_bar"] ==
            "QEMU fixture BAR4: 64-bit prefetchable memory", "wrong common BAR fixture")
    require(profile["iommu_platform"] is False and profile["ats"] is False,
            "M5 must not claim IOMMU/ATS fixture containment")
    require(profile["driver_features"] ==
            "zero; no feature selector or driver-feature register write",
            "M5 must keep zero driver features")
    require(profile["queue_index"] == 0, "M5 must select queue zero only")
    require(profile["queue_size_required"] == "non-zero", "M5 must require non-zero queue size")
    require(profile["final_device_status"] == 0, "M5 must record final reset status zero")

    image = bind(project, evidence["image"], "M5 queue-zero image")
    require(len(image) > 0, "empty M5 queue-zero image")
    sources = {label: bind(project, item, label)
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M5 disposable disk fixture")
    require(len(fixture) == 8 * 1024 * 1024, "M5 fixture must remain an 8 MiB raw file")
    require(set(fixture) <= {"\x00"}, "M5 fixture changed despite no block request claim")
    runtime = bind(project, evidence["runtime_evidence"], "M5 runtime log")

    pci = sources["pci_helper"]
    header = sources["pci_header"]
    wiring = sources["root_wiring"]
    cmake = sources["build_gate"]
    root_main = sources["root_main"]

    for required in (
        "SeLinRootVirtioBlkQueueZeroProbe",
        "SELINOS_ROOT_VIRTIO_BLK_QUEUE_ZERO_PROBE",
        "Enable root-only virtio queue-zero size observation/reset proof",
    ):
        require(required in cmake, f"missing M5 build-gate control: {required}")
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_QUEUE_ZERO_PROBE",
        "selinos_pci_inspect_qemu_virtio_blk_common_capability",
        "selinos_pci_validate_qemu_virtio_blk_common_range",
        "selinos_pci_observe_qemu_virtio_blk_queue_zero",
        "root-only queue-zero size observed then reset to zero",
        "no queue enable/address/notify, IRQ, DMA, bus mastering or block I/O",
    ):
        require(required in wiring, f"missing root M5 wiring control: {required}")
    require("selinos_domain_manager_start" in root_main,
            "M5 must execute only through root bootstrap")

    for required in (
        "struct selinos_qemu_virtio_queue_zero_observation",
        "queue_index", "queue_size", "status_after_reset",
        "selinos_pci_observe_qemu_virtio_blk_queue_zero",
        "temporary zero-feature DRIVER_OK state",
        "size/address/enable, notification, IRQ, DMA or block-I/O state",
    ):
        require(required in header, f"missing M5 public contract control: {required}")
    for required in (
        "VIRTIO_COMMON_QUEUE_SELECT_OFF 0x16u",
        "VIRTIO_COMMON_QUEUE_SIZE_OFF   0x18u",
        "VIRTIO_COMMON_QUEUE_ZERO       0u",
        "selinos_pci_observe_qemu_virtio_blk_queue_zero",
        "vka_alloc_frame_at", "vspace_reserve_range_aligned",
        "vspace_map_pages_at_vaddr", "vspace_unmap_pages", "VSPACE_PRESERVE",
        "vka_free_object", "observation->queue_index", "observation->queue_size",
    ):
        require(required in pci, f"missing M5 queue-zero control: {required}")
    stage = function_slice(pci, "bool selinos_pci_observe_qemu_virtio_blk_queue_zero")
    sequence = [
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;",
        "VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;",
        "*queue_select = VIRTIO_COMMON_QUEUE_ZERO;",
        "observation->queue_size = *queue_size;",
        "observation->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
    ]
    positions = [stage.index(fragment) for fragment in sequence]
    require(positions == sorted(positions), "M5 status/queue source order is invalid")
    require(stage.count("*queue_select = VIRTIO_COMMON_QUEUE_ZERO;") == 1,
            "M5 must select queue zero exactly once")
    require(stage.count("observation->queue_size = *queue_size;") == 1,
            "M5 must read queue size exactly once")
    require(stage.count("common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =") == 7,
            "M5 must contain six normal status writes plus one fail-closed reset")
    for forbidden in (
        "queue_enable", "queue_notify", "queue_desc", "queue_driver", "queue_device",
        "seL4_IRQHandler", "seL4_IRQControl", "seL4_X86_IOSpace", "seL4_X86_IOPort",
        "PCI_COMMAND_MASTER", "block request", "vka_alloc_dma", "sel4utils_copy_cap_to_process",
        "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF", "VIRTIO_COMMON_DEVICE_FEATURE_OFF",
    ):
        require(forbidden not in stage, f"forbidden M5 queue/I-O control: {forbidden}")

    contract = evidence["validated_contract"]
    require(contract["status_sequence"] ==
            "0 -> ACKNOWLEDGE -> ACKNOWLEDGE|DRIVER -> ACKNOWLEDGE|DRIVER|FEATURES_OK -> ACKNOWLEDGE|DRIVER|FEATURES_OK|DRIVER_OK -> queue_select=0/read queue_size -> 0",
            "wrong M5 status-sequence contract")
    for field, phrase in (("queue_scope", "exactly one queue_select write"),
                          ("mapping", "released"),
                          ("zero_feature_policy", "no device-feature selector"),
                          ("authority", "no cap copy")):
        require(phrase in contract[field], f"missing M5 contract assertion: {field}")

    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in runtime, f"missing M5 runtime marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M5 runtime marker: {marker}")
    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("configured or enabled virtqueue", "DMA", "block request", "persistent VFS",
                   "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M5 non-claim: {phrase}")

    print("SeLinOS virtio-blk root-only queue-zero metadata observation M5 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
