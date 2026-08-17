#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS KAPI M2 fail-closed ioremap evidence."""

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


def binding(project: Path, item: dict[str, str], label: str) -> Path:
    path = project / item["path"]
    require(path.is_file(), f"missing {label}: {path}")
    require(sha256(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    record = json.loads(
        (project / "tests/artifacts/selinos_kapi_m2_ioremap_failclosed.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M2 must remain native seL4 evidence")
    binding(project, record["image"], "M2 image")
    implementation = {
        label: binding(project, item, label)
        for label, item in record["implementation"].items()
    }
    log = binding(project, record["runtime_evidence"], "M2 runtime log")

    io_header = implementation["io_contract"].read_text(encoding="utf-8")
    core = implementation["kapi_core"].read_text(encoding="utf-8")
    probe = implementation["isolated_edu_probe"].read_text(encoding="utf-8")
    for fragment in ("void *ioremap", "void iounmap", "never returns\n * a device mapping"):
        require(fragment in io_header, f"missing M2 IO contract: {fragment}")
    core_start = core.find("void *ioremap(")
    core_end = core.find("void iounmap(", core_start)
    require(core_start >= 0 and core_end > core_start, "missing fail-closed ioremap implementation")
    core_ioremap = core[core_start:core_end]
    require("return NULL;" in core_ioremap, "ioremap must fail closed")
    for forbidden in ("vka_alloc_frame_at", "vspace_map_pages_at_vaddr", "seL4_X86_IRQControl", "seL4_Signal", "seL4_Wait"):
        require(forbidden not in core_ioremap, f"ioremap must not use {forbidden}")

    guard = probe[probe.find("static int edu_kapi_m2_ioremap_fails_closed"):probe.find("static int edu_probe(")]
    require("ioremap(physical_address, size) != NULL" in guard and "iounmap(NULL)" in guard,
            "missing isolated fail-closed lifecycle check")
    guard_call = probe.find("edu_kapi_m2_ioremap_fails_closed")
    enable_call = probe.find("pci_enable_device(pdev)")
    require(guard_call >= 0 and enable_call > guard_call,
            "M2 ioremap guard must precede device enable")
    require("KAPI M2 ioremap fails closed; no MMIO authority." in probe,
            "missing M2 authority-withheld marker")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M2 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M2 marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("device frame capability", "canonical request_irq/free_irq stub",
                   "NIC/storage authority", "dpkg or apt"):
        require(phrase in exclusions, f"missing M2 non-claim: {phrase}")
    print("SeLinOS KAPI M2 fail-closed ioremap evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
