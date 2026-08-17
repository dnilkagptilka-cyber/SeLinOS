#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS M13 root-tracked ARCH_GET_FS evidence."""

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
        (project / "tests/artifacts/selinos_linux_tls_arch_get_fs_m13.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M13 must remain native seL4 evidence")
    binding(project, record["image"], "M13 image")
    dispatcher = binding(project, record["implementation"]["root_dispatcher"],
                         "M13 root dispatcher")
    probe = binding(project, record["implementation"]["isolated_probe"],
                    "M13 isolated probe")
    log = binding(project, record["runtime_evidence"], "M13 runtime log")

    dispatcher_text = dispatcher.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_ARCH_GET_FS 0x1003u",
        "seL4_Word probe_tls_base = 0u;",
        "probe_tls_base = frame.values[seL4_UnknownSyscall_RSI];",
        "frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_ARCH_GET_FS",
        "probe_tls_base != SELINOS_LINUX_MMAP_ADDRESS",
        "copy_linux_word_to_user(vka, vspace, &probe",
        "root-tracked ARCH_GET_FS mapped TLS base returned",
    ):
        require(fragment in dispatcher_text, f"missing M13 dispatcher control: {fragment}")
    m13_start = dispatcher_text.find("/* M13 returns")
    m13_end = dispatcher_text.find("expected clock_gettime", m13_start)
    require(m13_start >= 0 and m13_end > m13_start, "missing M13 dispatcher branch")
    branch = dispatcher_text[m13_start:m13_end]
    for forbidden in ("seL4_TCB_SetTLSBase", "seL4_TCB_ReadRegisters", "seL4_Call"):
        require(forbidden not in branch, f"M13 must not perform {forbidden}")

    probe_text = probe.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_ARCH_GET_FS 0x1003ul",
        '"mov $0x1003, %%rdi\\n"',
        '"lea 32(%%r12), %%rsi\\n"',
        '"cmp %%r12, 32(%%r12)\\n"',
    ):
        require(fragment in probe_text, f"missing M13 probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M13 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M13 marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("general ARCH_GET_FS", "GS base", "ELF TLS", "dpkg, apt"):
        require(phrase in exclusions, f"missing M13 non-claim: {phrase}")
    print("SeLinOS Linux TLS ARCH_GET_FS M13 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
