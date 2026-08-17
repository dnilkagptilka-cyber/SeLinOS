#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS ROMFS syscall bridge M2 evidence."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RECORD = ROOT / "tests/artifacts/selinos_romfs_syscall_bridge_m2.verification.json"
LOG = ROOT / "tests/artifacts/selinos_romfs_syscall_bridge_m2.boot.log"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    image = ROOT / record["image"]["path"]
    subprocess.run(["ninja"], cwd=BUILD, check=True)
    require(sha256(image) == record["image"]["sha256"], "image hash mismatch")

    texts = {}
    for label, source in record["sources"].items():
        path = ROOT / source["path"]
        require(path.is_file(), f"missing {label} source")
        require(sha256(path) == source["sha256"], f"{label} source hash mismatch")
        texts[label] = path.read_text(encoding="utf-8")

    for token in ("SELINOS_LINUX_SYS_OPENAT", "SELINOS_LINUX_SYS_CLOSE",
                  "selinos_linux_release_path", "SELINOS_ROMFS_RELEASE_MAGIC"):
        require(token in texts["probe"], f"probe missing bridge token: {token}")
    for token in ("call_romfs_root", "copy_linux_word_to_user", "SELINOS_LINUX_OPENAT",
                  "SELINOS_LINUX_CLOSE", "restricted ROMFS openat rejected",
                  "restricted ROMFS read rejected", "restricted ROMFS close rejected"):
        require(token in texts["gateway"], f"gateway missing bridge token: {token}")
    require("sel4utils_dup_and_map" in texts["gateway"],
            "gateway must use temporary duplicate mapping for user-page copy")
    for token in ("SELINOS_ROMFS_OP_OPEN", "SELINOS_ROMFS_OP_READ", "SELINOS_ROMFS_OP_CLOSE"):
        require(token in texts["romfs_server"], f"ROMFS server missing token: {token}")

    command = (
        "timeout 12s qemu-system-x86_64 -cpu max -nographic -serial mon:stdio "
        "-m size=1G -kernel images/kernel-x86_64-pc99 "
        "-initrd images/selinos-root-image-x86_64-pc99"
    )
    result = subprocess.run(["script", "-qefc", command, str(LOG)], cwd=BUILD)
    require(result.returncode == 124,
            f"QEMU must remain live until timeout, got {result.returncode}")
    output = LOG.read_text(encoding="utf-8", errors="replace")
    markers = record["required_serial_markers"]
    for marker in markers:
        require(marker in output, f"missing serial marker: {marker}")
    for marker in record["forbidden_serial_markers"]:
        require(marker not in output, f"forbidden serial marker: {marker}")
    require(output.index(markers[0]) < output.index(markers[1]),
            "initial user write must precede syscall-to-ROMFS completion")
    print("SeLinOS ROMFS syscall bridge M2 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"SeLinOS ROMFS syscall bridge M2 verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
