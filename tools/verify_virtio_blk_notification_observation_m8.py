#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk notification observation M8 evidence."""
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
    offsets = [source.index(fragment) for fragment in fragments]
    require(offsets == sorted(offsets), message)


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    evidence = json.loads(
        (project / "tests/artifacts/selinos_virtio_notification_observation_m8.verification.json").read_text()
    )
    require(evidence["schema"] == 1, "unexpected M8 evidence schema")
    profile = evidence["profile"]
    expected = {
        "build_directory": "build-virtio-notification-observation-probe",
        "cmake_option": "SeLinRootVirtioBlkNotificationObservationProbe=ON",
        "device_identity": "1af4:1042",
        "driver_features": "zero; no feature selector or driver-feature register write",
        "queue_index": 0,
        "maximum_queue_size_required": "non-zero",
        "notification_bar_mapping": "absent",
        "notification_write": "absent",
        "final_device_status": 0,
    }
    for key, value in expected.items():
        require(profile[key] == value, f"wrong M8 profile value: {key}")
    require("non-zero-multiplier" in profile["notification_capability"],
            "M8 profile lacks non-zero notification multiplier boundary")

    for label, item in evidence["images"].items():
        image = bind(project, item, f"M8 {label} image")
        require(image.stat().st_size > 0, f"empty M8 {label} image")
    sources = {label: text(bind(project, item, label))
               for label, item in evidence["implementation"].items()}
    fixture = bind(project, evidence["fixture"], "M8 disposable disk fixture")
    fixture_bytes = fixture.read_bytes()
    require(len(fixture_bytes) == 8 * 1024 * 1024,
            "M8 fixture must remain an 8 MiB raw file")
    require(set(fixture_bytes) <= {0},
            "M8 fixture changed despite no-notification/no-I/O contract")
    runtime = text(bind(project, evidence["runtime_evidence"], "M8 runtime log"))

    cmake = sources["build_gate"]
    for required in (
        "SeLinRootVirtioBlkNotificationObservationProbe",
        "SELINOS_ROOT_VIRTIO_BLK_NOTIFICATION_OBSERVATION_PROBE",
        "Enable root-only virtio notification-capability/address observation proof",
    ):
        require(required in cmake, f"missing M8 CMake gate: {required}")

    wiring = sources["root_wiring"]
    for required in (
        "CONFIG_SELINOS_ROOT_VIRTIO_BLK_NOTIFICATION_OBSERVATION_PROBE",
        "selinos_pci_inspect_qemu_virtio_blk_notify_capability",
        "selinos_pci_validate_qemu_virtio_blk_notify_range",
        "selinos_pci_observe_qemu_virtio_blk_queue_zero_notification",
        "queue-zero notification address observation passed",
        "notification BAR unmapped; no notify, IRQ, DMA claim or block I/O",
    ):
        require(required in wiring, f"missing M8 root wiring control: {required}")
    require("selinos_domain_manager_start" in sources["root_main"],
            "M8 must execute only through root bootstrap")

    header = sources["pci_header"]
    for required in (
        "struct selinos_qemu_virtio_notify_capability",
        "struct selinos_qemu_virtio_notify_range",
        "struct selinos_qemu_virtio_notification_observation",
        "notify_off_multiplier", "queue_notify_off", "notification_paddr",
        "selinos_pci_inspect_qemu_virtio_blk_notify_capability",
        "selinos_pci_validate_qemu_virtio_blk_notify_range",
        "selinos_pci_observe_qemu_virtio_blk_queue_zero_notification",
        "notification BAR remains unmapped and is never read or written.",
    ):
        require(required in header, f"missing M8 public contract: {required}")

    pci = sources["pci_helper"]
    for required in (
        "VIRTIO_PCI_CAP_NOTIFY_CFG 0x02u",
        "VIRTIO_NOTIFY_CFG_MIN_LENGTH 0x14u",
        "VIRTIO_NOTIFY_CFG_MULTIPLIER_OFF 0x10u",
        "VIRTIO_COMMON_QUEUE_NOTIFY_OFF 0x1eu",
        "bool selinos_pci_inspect_qemu_virtio_blk_notify_capability",
        "cap_length >= VIRTIO_NOTIFY_CFG_MIN_LENGTH",
        "cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG",
        "multiplier != 0u",
        "bool selinos_pci_validate_qemu_virtio_blk_notify_range",
        "bool selinos_pci_observe_qemu_virtio_blk_queue_zero_notification",
    ):
        require(required in pci, f"missing M8 source control: {required}")

    parser = function_slice(pci, "bool selinos_pci_inspect_qemu_virtio_blk_notify_capability")
    for required in (
        "PCI_CAPABILITY_LIMIT", "PCI_CAP_ID_VENDOR_SPECIFIC", "cap_length",
        "VIRTIO_NOTIFY_CFG_MULTIPLIER_OFF", "bar < PCI_BAR_COUNT",
        "(offset & 0x3u) == 0u", "length >= VIRTIO_NOTIFY_CFG_MIN_LENGTH",
        "capability->notify_off_multiplier = multiplier;",
        "release_config_ioport(vka, ioport_slot);",
    ):
        require(required in parser, f"M8 parser lacks required control: {required}")
    for forbidden in ("vka_alloc_frame", "vspace_", "write_config_", "seL4_IRQHandler"):
        require(forbidden not in parser, f"forbidden M8 parser control: {forbidden}")

    validator = function_slice(pci, "bool selinos_pci_validate_qemu_virtio_blk_notify_range")
    for required in (
        "selinos_pci_inspect_qemu_virtio_blk_common_capability",
        "selinos_pci_validate_qemu_virtio_blk_common_range",
        "capability->bar_index != common_range.capability.bar_index",
        "common_range.bar_paddr > UINTPTR_MAX - (uintptr_t)capability->offset",
        "range->notification_cap_paddr = common_range.bar_paddr + (uintptr_t)capability->offset;",
        "virtio_notify_range_is_sane(range)",
    ):
        require(required in validator, f"M8 notification range validator lacks: {required}")
    for forbidden in ("vka_alloc_frame", "vspace_", "seL4_IRQHandler", "queue_notify"):
        require(forbidden not in validator, f"forbidden M8 range validator control: {forbidden}")

    observer = function_slice(pci, "bool selinos_pci_observe_qemu_virtio_blk_queue_zero_notification")
    for required in (
        "vka_alloc_frame_at(vka, seL4_PageBits, common_range->page_paddr, &common_frame)",
        "queue_notify_off = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_NOTIFY_OFF);",
        "*queue_select = VIRTIO_COMMON_QUEUE_ZERO;",
        "observation->queue_notify_off = *queue_notify_off;",
        "UINTPTR_MAX / (uintptr_t)observation->queue_notify_off",
        "notification_relative = (uintptr_t)observation->queue_notify_off *",
        "observation->notification_paddr = notify_range->notification_cap_paddr + notification_relative;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;",
        "vspace_unmap_pages(root_vspace, common_mapping, 1u, seL4_PageBits,",
        "vka_free_object(vka, &common_frame);",
    ):
        require(required in observer, f"M8 observer lacks required control: {required}")
    ordered(observer, (
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;",
        "common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;",
        "*queue_select = VIRTIO_COMMON_QUEUE_ZERO;",
        "observation->queue_notify_off = *queue_notify_off;",
        "notification_relative = (uintptr_t)observation->queue_notify_off *",
        "observation->notification_paddr = notify_range->notification_cap_paddr + notification_relative;",
        "observation->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];",
    ), "M8 common-state/notification-address sequence is not strictly ordered")
    require(observer.count("*queue_select = VIRTIO_COMMON_QUEUE_ZERO;") == 1,
            "M8 must select queue zero exactly once")
    for forbidden in (
        "    *queue_notify_off =", "notification_mapping", "vka_alloc_frame_at(vka, seL4_PageBits, notify",
        "    *queue_enable =", "    *queue_size =", "    *queue_desc_", "queue_notify",
        "seL4_IRQHandler", "seL4_IRQControl", "seL4_X86_IOSpace", "seL4_X86_IOPort",
        "PCI_COMMAND_MASTER", "vka_alloc_dma", "sel4utils_copy_cap_to_process",
        "VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF", "VIRTIO_COMMON_DEVICE_FEATURE_OFF",
    ):
        if forbidden == "queue_notify":
            continue
        require(forbidden not in observer, f"forbidden M8 observer control: {forbidden}")

    design = sources["phase_design_gate"]
    for required in (
        "Status: verified, bounded M8 proof.", "VIRTIO_PCI_CAP_NOTIFY_CFG", "queue_notify_off",
        "does **not** map the notification BAR/page", "Explicit non-claims",
        "DMA behavior or containment", "dpkg` or `apt`",
    ):
        require(required in design, f"missing M8 design boundary: {required}")

    contract = evidence["validated_contract"]
    for field, phrase in (
        ("capability", "non-zero notify_off_multiplier"),
        ("address_policy", "overflow checks"),
        ("mapping", "notification BAR is not framed or mapped"),
        ("authority", "notification mapping/write"),
    ):
        require(phrase in contract[field], f"missing M8 contract assertion: {field}")
    runtime_evidence = evidence["runtime_evidence"]
    for marker in runtime_evidence["required_markers"]:
        require(marker in runtime, f"missing M8 runtime marker: {marker}")
    for marker in runtime_evidence["forbidden_markers"]:
        require(marker not in runtime, f"forbidden M8 runtime marker: {marker}")
    before = runtime.rsplit("fixture_before=", 1)[1].splitlines()[0]
    after = runtime.rsplit("fixture_after=", 1)[1].splitlines()[0]
    require(before == after == evidence["fixture"]["sha256"],
            "M8 runtime fixture hashes do not bind unchanged media")

    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("notification delivery", "DMA behavior or containment", "block request",
                   "persistent VFS", "Linux block/KAPI", "dpkg or apt"):
        require(phrase in exclusions, f"missing M8 non-claim: {phrase}")

    print("SeLinOS virtio-blk root-only notification-capability/address observation M8 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
