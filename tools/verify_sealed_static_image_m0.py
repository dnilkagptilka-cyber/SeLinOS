#!/usr/bin/env python3
"""Independent verifier for Phase 64’s non-executing SSIM-v1 parser gate."""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    record = json.loads(
        (root / "tests/artifacts/selinos_sealed_static_image_m0.verification.json").read_text()
    )
    require(record["profile"]["cmake_option"] == "SeLinSealedStaticImageProbe=ON", "profile")
    for group in ("images", "fixtures", "implementation"):
        for label, binding in record[group].items():
            path = root / binding["path"]
            require(path.is_file(), f"missing {group}:{label}")
            require(sha256(path) == binding["sha256"], f"hash {group}:{label}")
    runtime = root / record["runtime"]["path"]
    require(runtime.is_file() and sha256(runtime) == record["runtime"]["sha256"],
            "runtime hash")
    runtime_text = runtime.read_text(errors="replace")
    for marker in record["runtime"]["required_markers"]:
        require(marker in runtime_text, f"runtime marker: {marker}")
    require(runtime_text.index(record["runtime"]["required_markers"][0]) <
            runtime_text.index(record["runtime"]["required_markers"][3]),
            "accept before probe marker")

    subprocess.run([sys.executable, "tools/generate_sealed_static_image_m0_fixture.py"],
                   cwd=root, check=True, stdout=subprocess.DEVNULL)
    fixture = root / "tests/artifacts/sealed_static_image_m0/valid.ssim"
    ledger = json.loads(
        (root / "tests/artifacts/sealed_static_image_m0/valid.ledger.json").read_text()
    )
    require(sha256(fixture) == ledger["wire_sha256"], "fixture wire ledger")
    require(ledger["source_bytes"] == 99 and ledger["entry_vaddr"] == "0x0000000060000000",
            "fixture canonical ledger")
    require(ledger["permissions"] == "R|X" and ledger["payload_bytes"] == 3,
            "fixture W^X ledger")

    host_binary = root / "tests/artifacts/sealed_static_image_m0/build/sealed_static_image_m0_test"
    host_binary.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-Isrc/projects/helixos/include", "-Isrc/projects/helixos/kabi/include",
        "tests/sealed_static_image_m0_test.c",
        "src/projects/helixos/src/sealed_static_image_m0.c",
        "src/projects/helixos/kabi/src/selinos_kabi_policy.c",
        "-o", str(host_binary),
    ], cwd=root, check=True)
    host = subprocess.run([str(host_binary), str(fixture)], cwd=root,
                          check=True, text=True, capture_output=True)
    require("valid ledger plus all negative guards passed" in host.stdout,
            "host negative parser marker")

    parser = (root / record["implementation"]["parser"]["path"]).read_text()
    header = (root / record["implementation"]["parser_header"]["path"]).read_text()
    server = (root / record["implementation"]["server"]["path"]).read_text()
    probe = (root / record["implementation"]["probe"]["path"]).read_text()
    manager = (root / record["implementation"]["root"]["path"]).read_text()
    cmake = (root / record["implementation"]["cmake"]["path"]).read_text()
    gate = (root / record["implementation"]["gate"]["path"]).read_text()
    for marker in (
        "SELINOS_SEALED_STATIC_IMAGE_M0_HEADER_BYTES",
        "SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX",
        "SELINOS_SEALED_STATIC_IMAGE_M0_ENTRY_VADDR",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE",
    ):
        require(marker in header, f"header marker: {marker}")
    for marker in (
        "checked_range", "selinos_kabi_sha256", "segment_file_bytes != segment_memory_bytes",
        "permissions != SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX",
        "read_u16(segment + 22u)", "zeroed_source",
    ):
        require(marker in parser, f"parser marker: {marker}")
    for marker in (
        "SeLinSealedStaticImageProbe", "SELINOS_SEALED_STATIC_IMAGE_PROBE",
        "selinos-sealed-static-image-m0-parser", "selinos-sealed-static-image-m0-probe",
    ):
        require(marker in cmake, f"cmake marker: {marker}")
    for marker in (
        "start_sealed_static_image_m0", "vka_alloc_endpoint", "vka_alloc_notification",
        "sel4utils_spawn_process_v",
    ):
        require(marker in manager, f"manager marker: {marker}")
    for marker in (
        "negative_guards_pass", "SELINOS_SEALED_STATIC_IMAGE_M0_E_DIGEST",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_PERMISSION",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_ADDRESS",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_RESERVED",
        "SELINOS_SEALED_STATIC_IMAGE_M0_E_RANGE",
    ):
        require(marker in server, f"server marker: {marker}")
    require("SELINOS_SEALED_STATIC_IMAGE_M0_ACCEPTED" in probe and
            "SELINOS_SEALED_STATIC_IMAGE_M0_REJECTED" in probe,
            "probe accept/reject ledger")
    static_surface = parser + server + probe
    for forbidden in ("seL4_TCB_Resume", "sel4utils_map_page", "vka_alloc_frame",
                      "seL4_X86_Page_Map", "sel4utils_spawn_process_v", "fork(",
                      "clone(", "pthread_"):
        require(forbidden not in static_surface,
                f"forbidden runtime surface: {forbidden}")
    require("does not prove" in gate and "ELF" in gate and "Linux ABI" in gate,
            "gate scope")
    print("SeLinOS sealed static-image M0 evidence verified.")


try:
    main()
except (OSError, RuntimeError, subprocess.CalledProcessError, KeyError, ValueError) as error:
    print(f"verification failed: {error}", file=sys.stderr)
    sys.exit(1)
