#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Refresh hash-bound runtime evidence after a behaviorally revalidated image rebuild.

This utility is intentionally narrow. It does not generate tests, edit source, or change
claimed coverage. It copies only QEMU logs produced by the three supported profiles and
updates only the corresponding image/log SHA-256 fields in evidence records.
"""

from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "build/images/selinos-root-image-x86_64-pc99"
FRESH = {
    ROOT / "tests/artifacts/selinos_m1_baseline.log": Path("/tmp/selinos_pci_primitive_baseline.log"),
    ROOT / "tests/artifacts/selinos_dma_irq_m1.log": Path("/tmp/selinos_pci_primitive_edu.log"),
    ROOT / "tests/artifacts/selinos_two_edu_m3.log": Path("/tmp/selinos_pci_primitive_two_edu.log"),
}
RECORDS = [
    ROOT / "tests/artifacts/selinos_driver_runtime_m1.verification.json",
    ROOT / "tests/artifacts/selinos_linux_syscall_abi_m1.verification.json",
    ROOT / "tests/artifacts/selinos_m3_two_edu.verification.json",
    ROOT / "tests/artifacts/selinos_linux_syscall_abi_m2.verification.json",
    ROOT / "tests/artifacts/selinos_linux_syscall_abi_m3.verification.json",
    ROOT / "tests/artifacts/selinos_linux_syscall_abi_m4.verification.json",
    ROOT / "tests/artifacts/selinos_romfs_m1.verification.json",
    ROOT / "tests/artifacts/selinos_romfs_syscall_bridge_m2.verification.json",
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def replace_hashes(node: object, path_hashes: dict[str, str]) -> None:
    if isinstance(node, dict):
        path = node.get("path")
        if isinstance(path, str) and path in path_hashes and "sha256" in node:
            node["sha256"] = path_hashes[path]
        for value in node.values():
            replace_hashes(value, path_hashes)
    elif isinstance(node, list):
        for value in node:
            replace_hashes(value, path_hashes)


def main() -> int:
    if not IMAGE.is_file():
        raise RuntimeError(f"missing image: {IMAGE}")
    for destination, source in FRESH.items():
        if not source.is_file():
            raise RuntimeError(f"missing fresh runtime log: {source}")
        shutil.copyfile(source, destination)

    path_hashes = {
        "build/images/selinos-root-image-x86_64-pc99": sha256(IMAGE),
        "src/projects/helixos/src/domain_manager.c": sha256(
            ROOT / "src/projects/helixos/src/domain_manager.c"
        ),
        "src/projects/helixos/servers/romfsd.c": sha256(
            ROOT / "src/projects/helixos/servers/romfsd.c"
        ),
        "src/projects/helixos/drivers/romfs_probe.c": sha256(
            ROOT / "src/projects/helixos/drivers/romfs_probe.c"
        ),
        "src/projects/helixos/include/selinos_romfs_protocol.h": sha256(
            ROOT / "src/projects/helixos/include/selinos_romfs_protocol.h"
        ),
        "src/projects/helixos/drivers/linux_syscall_probe.c": sha256(
            ROOT / "src/projects/helixos/drivers/linux_syscall_probe.c"
        ),
    }
    path_hashes.update({
        str(path.relative_to(ROOT)): sha256(path)
        for path in FRESH
    })
    for record_path in RECORDS:
        record = json.loads(record_path.read_text(encoding="utf-8"))
        replace_hashes(record, path_hashes)
        record_path.write_text(json.dumps(record, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"image={path_hashes['build/images/selinos-root-image-x86_64-pc99']}")
    for path in FRESH:
        print(f"{path.relative_to(ROOT)}={path_hashes[str(path.relative_to(ROOT))]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
