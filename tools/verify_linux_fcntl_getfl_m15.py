#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def bind(root: Path, item: dict, label: str) -> Path:
    path = root / item["path"]
    need(path.is_file(), f"missing {label}")
    if "sha256" in item:
        need(digest(path) == item["sha256"], f"{label} SHA-256 mismatch")
    return path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    record = json.loads(
        (root / "tests/artifacts/selinos_linux_fcntl_getfl_m15.verification.json").read_text()
    )
    need(record["schema"] == 1, "invalid M15 evidence schema")
    need(record["platform"]["linux_kernel_present"] is False, "invalid M15 platform evidence")
    bind(root, record["image"], "image")
    dispatcher = bind(root, record["implementation"]["root_dispatcher"], "dispatcher").read_text()
    probe = bind(root, record["implementation"]["isolated_probe"], "probe").read_text()
    guard = bind(root, record["implementation"]["object_window_guard"], "object window guard")
    log = bind(root, record["runtime_evidence"], "runtime log").read_text(errors="replace")

    for required in (
        "SELINOS_LINUX_FCNTL 72u",
        "SELINOS_LINUX_F_GETFL 3u",
        "frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION",
        "frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_F_GETFL",
        "frame.values[seL4_UnknownSyscall_RDX] != 0u",
        "fixed immutable FD F_GETFL mediated without VFS IPC",
    ):
        need(required in dispatcher, f"missing dispatcher control: {required}")

    for required in (
        "SELINOS_LINUX_SYS_FCNTL 72ul",
        "SELINOS_LINUX_F_GETFL 3ul",
        '"mov $72, %%rax\\n"',
        '"mov $6, %%rdi\\n"',
        '"mov $3, %%rsi\\n"',
        '"test %%rax, %%rax\\n"',
    ):
        need(required in probe, f"missing probe control: {required}")

    expected_window = (
        '"mov $72, %%rax\\n"\n'
        '        "mov $6, %%rdi\\n"\n'
        '        "mov $3, %%rsi\\n"\n'
        '        "xor %%rdx, %%rdx\\n"\n'
        '        "syscall\\n"\n'
        '        "test %%rax, %%rax\\n"\n'
        '        "jnz 1f\\n"\n'
        '        "mov $6, %%rdi\\n"\n'
        '        "mov %%r12, %%rsi\\n"\n'
        '        "mov %[fstat], %%rax\\n"'
    )
    need(expected_window in probe, "M15 probe does not restore FD 6 before M10 fstat")

    candidate = root / "build/projects/helixos/CMakeFiles/selinos-linux-syscall-probe.dir/drivers/linux_syscall_probe.c.obj"
    baseline = bind(root, record["implementation"]["m14_probe_baseline"], "preserved M14 probe object")
    need(candidate.is_file(), "missing M15 candidate probe object")
    run = subprocess.run([str(guard), str(baseline), str(candidate)], cwd=root,
                         text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    need(run.returncode == 0, "M15 object-window guard failed")

    for marker in record["runtime_evidence"]["required_markers"]:
        need(marker in log, f"missing marker: {marker}")
    for marker in record["runtime_evidence"]["forbidden_markers"]:
        need(marker not in log, f"forbidden marker: {marker}")

    claims = " ".join(record["not_claimed"])
    for excluded in ("general fcntl", "general VFS", "dpkg or apt"):
        need(excluded in claims, f"missing non-claim: {excluded}")

    print("SeLinOS Linux fcntl(F_GETFL) M15 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as error:
        print(f"verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
