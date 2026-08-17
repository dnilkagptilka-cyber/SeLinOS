#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS M11 getdents64 fail-closed rejection evidence."""

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
        (project / "tests/artifacts/selinos_linux_getdents_rejection_m11.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M11 must remain native seL4 evidence")
    binding(project, record["image"], "M11 image")
    dispatcher = binding(project, record["implementation"]["root_dispatcher"],
                         "M11 root dispatcher")
    probe = binding(project, record["implementation"]["isolated_probe"],
                    "M11 isolated probe")
    log = binding(project, record["runtime_evidence"], "M11 runtime log")

    text = dispatcher.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_GETDENTS64 217u",
        "SELINOS_LINUX_ENOTDIR 20u",
        "frame.values[seL4_UnknownSyscall_RDX] != 32u",
        "reply_linux_x86_64_syscall(&frame, (seL4_Word)-SELINOS_LINUX_ENOTDIR)",
        "getdents64(FD 6) rejected with ENOTDIR; no directory service",
    ):
        require(fragment in text, f"missing M11 dispatcher control: {fragment}")
    branch = text[text.find("/* M11 proves"):text.find("expected version close UnknownSyscall")]
    for forbidden in ("copy_linux_word_to_user", "copy_linux_write_buffer", "call_romfs_root"):
        require(forbidden not in branch, f"M11 rejection must not use {forbidden}")

    probe_text = probe.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_SYS_GETDENTS64 217ul",
        "SELINOS_LINUX_ENOTDIR 20ul",
        '"mov $217, %%rax\\n"',
        '"cmp $-20, %%eax\\n"',
    ):
        require(fragment in probe_text, f"missing M11 probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M11 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M11 marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("directory FD", "linux_dirent64", "general VFS", "dpkg, apt"):
        require(phrase in exclusions, f"missing M11 non-claim: {phrase}")
    print("SeLinOS Linux syscall ABI M11 getdents64 rejection evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
