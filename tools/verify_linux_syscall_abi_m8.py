#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS Linux syscall ABI M8 bounded evidence."""

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
        (project / "tests/artifacts/selinos_linux_syscall_abi_m8.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M8 must remain a native seL4 proof")

    verify_binding(project, record["image"], "M8 image")
    dispatcher = verify_binding(project, record["implementation"]["root_dispatcher"],
                                "M8 root dispatcher")
    probe = verify_binding(project, record["implementation"]["isolated_probe"],
                           "M8 isolated probe")
    log = verify_binding(project, record["runtime_evidence"], "M8 runtime log")

    dispatcher_text = dispatcher.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_MPROTECT 10u",
        "SELINOS_LINUX_LSEEK 8u",
        "SELINOS_LINUX_PREAD64 17u",
        "SELINOS_LINUX_MMAP_ADDRESS",
        "SELINOS_LINUX_SEEK_SET",
        "frame.values[seL4_UnknownSyscall_R10] != 0u",
        "SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE",
        "mapped-page mprotect acknowledgement, zero-origin lseek and fixed-offset volatile pread64 mediated",
    ):
        require(fragment in dispatcher_text, f"missing bounded M8 dispatcher control: {fragment}")
    require("vspace_map_pages_at_vaddr" not in dispatcher_text[dispatcher_text.find("M8 is deliberately"):],
            "M8 must not introduce a new generic VSpace mapping path")

    probe_text = probe.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_SYS_MPROTECT 10ul",
        "SELINOS_LINUX_SYS_LSEEK 8ul",
        "SELINOS_LINUX_SYS_PREAD64 17ul",
        "[mprotect] \"i\"(SELINOS_LINUX_SYS_MPROTECT)",
        "[lseek] \"i\"(SELINOS_LINUX_SYS_LSEEK)",
        "[pread64] \"i\"(SELINOS_LINUX_SYS_PREAD64)",
        "mov $5, %%rdi",
        "xor %%r10, %%r10",
    ):
        require(fragment in probe_text, f"missing M8 isolated probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M8 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M8 marker: {marker}")

    contract = record["contract"]
    for key in ("mprotect", "lseek", "pread64", "negative_scope"):
        require(key in contract and contract[key], f"missing M8 contract section: {key}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("general mprotect", "stateful lseek", "pread64 on arbitrary FDs",
                   "TLS/ARCH_SET_FS", "dpkg or apt"):
        require(phrase in exclusions, f"missing M8 non-claim: {phrase}")

    print("SeLinOS Linux syscall ABI M8 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
