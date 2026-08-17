#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify SeLinOS M12 ARCH_SET_FS mapped-page TLS evidence."""

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
        (project / "tests/artifacts/selinos_linux_tls_arch_prctl_m12.verification.json")
        .read_text(encoding="utf-8")
    )
    require(record["schema"] == 1, "unexpected evidence schema")
    require(record["platform"]["linux_kernel_present"] is False,
            "M12 must remain native seL4 evidence")
    require(record["platform"]["kernel_fsgsbase"] == "msr",
            "unexpected kernel FS/GS configuration")
    binding(project, record["image"], "M12 image")
    dispatcher = binding(project, record["implementation"]["root_dispatcher"],
                         "M12 root dispatcher")
    probe = binding(project, record["implementation"]["isolated_probe"],
                    "M12 isolated probe")
    log = binding(project, record["runtime_evidence"], "M12 runtime log")

    dispatcher_text = dispatcher.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_ARCH_PRCTL 158u",
        "SELINOS_LINUX_ARCH_SET_FS 0x1002u",
        "frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_MMAP_ADDRESS",
        "has_linux_user_word_page(&probe, frame.values[seL4_UnknownSyscall_RSI])",
        "seL4_TCB_SetTLSBase(probe.thread.tcb.cptr",
        "ARCH_SET_FS mapped-page TLS base and fs:0 load/store mediated",
    ):
        require(fragment in dispatcher_text, f"missing M12 dispatcher control: {fragment}")
    require("probe.thread.tcb.cptr" in dispatcher_text,
            "root must use retained target TCB cap")

    probe_text = probe.read_text(encoding="utf-8")
    for fragment in (
        "SELINOS_LINUX_SYS_ARCH_PRCTL 158ul",
        "SELINOS_LINUX_ARCH_SET_FS 0x1002ul",
        '"mov $158, %%rax\\n"',
        '"mov $0x1002, %%rdi\\n"',
        '"mov %%r8, %%fs:0\\n"',
        '"mov %%fs:0, %%r9\\n"',
        '"mov $0x544c5353454c494e, %%r8\\n"',
    ):
        require(fragment in probe_text, f"missing M12 probe control: {fragment}")

    output = log.read_text(encoding="utf-8", errors="replace")
    for marker in record["runtime_evidence"]["required_markers"]:
        require(marker in output, f"missing required M12 marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        require(marker not in output, f"forbidden M12 marker: {marker}")
    exclusions = " ".join(record["not_claimed"])
    for phrase in ("ARCH_GET_FS", "TCB capability delegation", "clone/fork", "dpkg or apt"):
        require(phrase in exclusions, f"missing M12 non-claim: {phrase}")
    print("SeLinOS Linux TLS ARCH_SET_FS M12 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, RuntimeError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
