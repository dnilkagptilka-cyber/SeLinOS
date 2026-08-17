#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Phase 13 root-only e1000 firmware BAR range N1 evidence."""

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


def verify_binding(project: Path, binding: dict[str, str], label: str) -> Path:
    path = project / binding["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == binding["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record_path = project / "tests/artifacts/selinos_network_e1000_bar_range_n1.verification.json"
    record = json.loads(record_path.read_text(encoding="utf-8"))

    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "N1 must remain a native seL4 proof")
    require(record["platform"]["iommu_detected"] is False,
            "N1 must retain the zero-IOMMU TCG containment stop rule")
    require(record["opt_in"]["cmake_option"] == "SeLinRootE1000BarRangeProbe=ON",
            "unexpected N1 opt-in gate")
    require(record["opt_in"]["default_production_option"] == "OFF",
            "N1 must remain disabled by default")

    verify_binding(project, record["image"], "N1 image")
    implementation = {
        label: verify_binding(project, binding, label)
        for label, binding in record["implementation"].items()
    }
    log = verify_binding(project, record["runtime_evidence"], "N1 runtime log")

    identity = record["identity"]
    require(identity["vendor_id"] == "8086" and identity["device_id"] == "100e" and
            identity["bar_index"] == 0, "unexpected e1000 BAR identity")

    pci_source = implementation["pci_source"].read_text(encoding="utf-8")
    helper_start = pci_source.find("bool selinos_pci_read_qemu_e1000_bar_range(")
    helper_end = pci_source.find("bool selinos_pci_find_qemu_e1000(", helper_start)
    require(helper_start >= 0 and helper_end > helper_start, "missing bounded N1 helper")
    helper = pci_source[helper_start:helper_end]
    for fragment in (
        "PCI_BAR_MEMORY_TYPE_32",
        "PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER",
        "PCI_BAR0_OFF, 0xffffffffu",
        "restored_bar != original_bar",
        "restored_command != original_command",
        "resource->paddr = paddr",
        "resource->size = (size_t)size64",
        "resource->bar_index = 0u",
    ):
        require(fragment in helper, f"missing N1 validation/restore control: {fragment}")
    for forbidden in (
        "selinos_pci_assign_unconfigured_bar32",
        "vka_alloc_frame_at",
        "vspace_map_pages_at_vaddr",
        "seL4_X86_IRQControl",
    ):
        require(forbidden not in helper, f"N1 helper must not perform: {forbidden}")

    contract = implementation["pci_contract"].read_text(encoding="utf-8")
    root_hook = implementation["root_hook"].read_text(encoding="utf-8")
    cmake = implementation["cmake_gate"].read_text(encoding="utf-8")
    require("selinos_pci_read_qemu_e1000_bar_range" in contract,
            "N1 scalar-only public contract missing")
    require("CONFIG_SELINOS_ROOT_E1000_BAR_RANGE_PROBE" in root_hook,
            "N1 root hook missing")
    require("no BAR frame, mapping, IRQ, DMA, bus mastering or packet I/O" in root_hook,
            "N1 explicit authority-withheld runtime marker missing")
    require("SeLinRootE1000BarRangeProbe" in cmake and "DEFAULT\n    OFF" in cmake,
            "N1 default-OFF gate missing")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required N1 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden N1 marker: {marker}")

    exclusions = " ".join(record["not_claimed"])
    for phrase in ("BAR mapping", "DMA", "NIC driver domain", "IOMMU or IOSpace", "dpkg or apt"):
        require(phrase in exclusions, f"missing N1 non-claim: {phrase}")

    print("SeLinOS network e1000 BAR range N1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
