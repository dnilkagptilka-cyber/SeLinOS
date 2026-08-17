#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Linux syscall ABI M10 probe-local fstat record evidence."""

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
        (project / "tests/artifacts/selinos_linux_fstat_record_m10.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M10 must remain native seL4 evidence")
    binding(project, record["image"], "M10 image")
    dispatcher = binding(project, record["implementation"]["root_dispatcher"],
                         "M10 root dispatcher")
    probe = binding(project, record["implementation"]["isolated_probe"],
                    "M10 isolated probe")
    log = binding(project, record["runtime_evidence"], "M10 runtime log")

    dispatcher_text = dispatcher.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_FSTAT 5u",
        "SELINOS_LINUX_M10_STAT_MAGIC 0x53454c5354415430ull",
        "SELINOS_ROMFS_FD_VERSION",
        "SELINOS_LINUX_M10_STAT_SIZE",
        "FD 6 probe-local two-word fstat record mediated; not Linux struct stat",
    ):
        require(fragment in dispatcher_text, f"missing M10 dispatcher control: {fragment}")
    probe_text = probe.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_SYS_FSTAT 5ul",
        "SELINOS_LINUX_M10_STAT_MAGIC 0x53454c5354415430ul",
        "[fstat] \"i\"(SELINOS_LINUX_SYS_FSTAT)",
        "mov $0x53454c5354415430, %%r8",
        "cmpq $8, 8(%%r12)",
    ):
        require(fragment in probe_text, f"missing M10 isolated probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M10 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M10 marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("Linux struct stat", "arbitrary file descriptors", "metadata", "dpkg or apt"):
        require(phrase in exclusions, f"missing M10 non-claim: {phrase}")
    print("SeLinOS Linux syscall ABI M10 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
