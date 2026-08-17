#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS root-only virtio-blk BAR/feature observation M2 evidence."""

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
    evidence_path = project / "tests/artifacts/selinos_virtio_blk_bar_feature_m2.verification.json"
    evidence = json.loads(evidence_path.read_text(encoding="utf-8"))

    require(evidence["schema"] == 1, "unexpected evidence schema")
    profile = evidence["profile"]
    require(profile["cmake_option"] == "SeLinRootVirtioBlkBarFeatureProbe=ON",
            "wrong M2 experiment gate")
    require(profile["device_identity"] == "1af4:1042", "wrong virtio device identity")
    require(profile["common_configuration_bar"] ==
            "QEMU fixture BAR4: 64-bit prefetchable memory",
            "M2 fixture must record the observed 64-bit BAR")
    require(profile["iommu_platform"] is False and profile["ats"] is False,
            "M2 must not claim an IOMMU/ATS fixture")

    verify_binding(project, evidence["image"], "opt-in M2 image")
    for label, binding in evidence["implementation"].items():
        verify_binding(project, binding, label)
    runtime = verify_binding(project, evidence["runtime_evidence"], "M2 runtime log")
    output = runtime.read_text(encoding="utf-8", errors="replace")
    for marker in evidence["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M2 marker: {marker}")
    for marker in evidence["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M2 marker: {marker}")

    contract = evidence["validated_contract"]
    require("32-bit or 64-bit" in contract["pci_bar"],
            "M2 must retain explicit 32/64-bit BAR validation")
    require("uncached and read-only" in contract["mmio"],
            "M2 must record uncached read-only root mapping")
    require("VSPACE_PRESERVE" in contract["teardown"],
            "M2 must record non-owning unmap teardown")
    require("no selector write" in contract["feature_window"],
            "M2 must state the no-selector-write limit")

    exclusions = " ".join(evidence["not_claimed"])
    for phrase in ("driver domain", "FEATURES_OK", "DRIVER_OK", "virtqueue",
                   "DMA", "IOMMU", "block request", "dpkg or apt"):
        require(phrase in exclusions, f"missing M2 non-claim: {phrase}")

    print("SeLinOS virtio-blk BAR/feature M2 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
