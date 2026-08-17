#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Phase 13 root-only e1000 discovery N0 evidence."""

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
    record = json.loads((project / "tests/artifacts/selinos_network_e1000_discovery_n0.verification.json")
                        .read_text(encoding="utf-8"))
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "N0 must remain a native seL4 proof")
    require(record["opt_in"]["cmake_option"] == "SeLinRootE1000DiscoveryProbe=ON",
            "unexpected N0 opt-in gate")
    require(record["opt_in"]["default_production_option"] == "OFF",
            "N0 must remain disabled by default")
    verify_binding(project, record["image"], "N0 image")
    for label, binding in record["implementation"].items():
        verify_binding(project, binding, label)
    log = verify_binding(project, record["runtime_evidence"], "N0 runtime log")
    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required N0 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden N0 marker: {marker}")
    identity = record["identity"]
    require(identity["vendor_id"] == "8086" and identity["device_id"] == "100e",
            "unexpected e1000 identity")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("BAR mapping", "DMA", "NIC driver domain", "IP", "dpkg or apt"):
        require(phrase in exclusions, f"missing N0 non-claim: {phrase}")
    print("SeLinOS network e1000 discovery N0 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
