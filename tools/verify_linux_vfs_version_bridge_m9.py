#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS immutable VFS M9 exact Linux version bridge evidence."""

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
    record = json.loads(
        (project / "tests/artifacts/selinos_linux_vfs_version_bridge_m9.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M9 must remain a native seL4 proof")
    verify_binding(project, record["image"], "M9 image")
    implementation = {
        label: verify_binding(project, binding, label)
        for label, binding in record["implementation"].items()
    }
    log = verify_binding(project, record["runtime_evidence"], "M9 runtime log")

    generator = implementation["cpio_generator"].read_text(encoding="utf-8")
    protocol = implementation["protocol"].read_text(encoding="utf-8")
    server = implementation["romfs_server"].read_text(encoding="utf-8")
    bridge = implementation["root_bridge"].read_text(encoding="utf-8")
    probe = implementation["isolated_probe"].read_text(encoding="utf-8")

    require("(\"/selinos-version\", b\"SELINOS9\")" in generator,
            "CPIO generator does not define fixed version record")
    for fragment in (
        "SELINOS_ROMFS_FILE_VERSION 4u",
        "SELINOS_ROMFS_FD_VERSION 6u",
        "SELINOS_ROMFS_VERSION_LENGTH 8u",
        "SELINOS_ROMFS_VERSION_MAGIC 0x53454c494e4f5339ull",
    ):
        require(fragment in protocol, f"missing M9 protocol constant: {fragment}")
    for fragment in (
        '"/selinos-version"',
        "SELINOS_ROMFS_FILE_VERSION",
        "SELINOS_ROMFS_FD_VERSION",
        "SELINOS_ROMFS_VERSION_MAGIC",
    ):
        require(fragment in server, f"missing M9 server control: {fragment}")
    for fragment in (
        '"/selinos-version"',
        "SELINOS_ROMFS_FD_VERSION",
        "SELINOS_ROMFS_VERSION_LENGTH",
        "SELINOS_ROMFS_VERSION_MAGIC",
        "exact immutable /selinos-version open/read/close bridge mediated",
    ):
        require(fragment in bridge, f"missing M9 root bridge control: {fragment}")
    for fragment in (
        "selinos_linux_version_path[] = \"/selinos-version\"",
        "SELINOS_ROMFS_VERSION_FD 6ul",
        "SELINOS_ROMFS_VERSION_MAGIC 0x53454c494e4f5339ul",
        "cmp $6, %%rax",
        "mov $0x53454c494e4f5339, %%r8",
    ):
        require(fragment in probe, f"missing M9 isolated probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M9 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M9 marker: {marker}")

    exclusions = " ".join(record["not_claimed"])
    for phrase in ("arbitrary pathname", "directories", "persistent writes", "dpkg or apt"):
        require(phrase in exclusions, f"missing M9 non-claim: {phrase}")
    print("SeLinOS immutable VFS M9 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
