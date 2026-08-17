#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Independently verify SeLinOS ROMFS/VFS foundation M1 evidence."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
RECORD = ROOT / "tests/artifacts/selinos_romfs_m1.verification.json"
LOG = ROOT / "tests/artifacts/selinos_romfs_m1.boot.log"


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

    require("SELINOS_ROMFS_ENDPOINT_SLOT 8u" in texts["protocol"],
            "ROMFS endpoint slot must be fixed at first copied capability slot")
    require("vka_alloc_endpoint" in texts["root_wiring"] and
            "sel4utils_copy_cap_to_process" in texts["root_wiring"],
            "root must explicitly allocate and copy the ROMFS endpoint")
    require("server_slot != SELINOS_ROMFS_ENDPOINT_SLOT" in texts["root_wiring"] and
            "client_slot != SELINOS_ROMFS_ENDPOINT_SLOT" in texts["root_wiring"],
            "root must validate both process endpoint slots")
    for token in ("SELINOS_ROMFS_OP_OPEN", "SELINOS_ROMFS_OP_READ",
                  "SELINOS_ROMFS_OP_CLOSE", "SELINOS_ROMFS_STATUS_EBADF"):
        require(token in texts["server"], f"ROMFS server missing protocol token: {token}")
    require("invalid-FD guard passed" in texts["probe"],
            "ROMFS probe must test invalid descriptor guard")

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
            "ROMFS server must be online before its client proof")
    print("SeLinOS ROMFS M1 evidence verified.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as error:
        print(f"SeLinOS ROMFS M1 verification failed: {error}", file=sys.stderr)
        raise SystemExit(1)
