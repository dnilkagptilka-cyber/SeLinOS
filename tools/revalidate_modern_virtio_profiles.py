#!/usr/bin/env python3
"""Revalidate modern virtio-blk evidence with the declared QEMU 1af4:1042 fixture.

Generic no-device replays are intentionally unsuitable for these profiles.  This
runner uses a bounded fresh QEMU run per isolated build directory and refreshes
only that manifest's current image/runtime SHA bindings after the run completes.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
FIXTURE = ARTIFACTS / "selinos_virtio_fixture.raw"
PROFILES = [
    ("build-virtio-discovery-probe", "selinos_virtio_blk_discovery_m0.verification.json"),
    ("build-virtio-capability-probe", "selinos_virtio_blk_common_cap_m1.verification.json"),
    ("build-virtio-bar-feature-probe", "selinos_virtio_blk_bar_feature_m2.verification.json"),
    ("build-virtio-feature-stage-probe", "selinos_virtio_blk_feature_stage_m3.verification.json"),
    ("build-virtio-driver-ready-probe", "selinos_virtio_blk_driver_ready_m4.verification.json"),
    ("build-virtio-queue-zero-probe", "selinos_virtio_blk_queue_zero_m5.verification.json"),
    ("build-virtio-queue-layout-probe", "selinos_virtio_queue_layout_m6.verification.json"),
    ("build-virtio-queue-enable-probe", "selinos_virtio_queue_enable_m7.verification.json"),
    ("build-virtio-notification-observation-probe", "selinos_virtio_notification_observation_m8.verification.json"),
    ("build-virtio-zero-descriptor-notification-probe", "selinos_virtio_zero_descriptor_notification_m9.verification.json"),
    ("build-virtio-zero-index-observation-probe", "selinos_virtio_zero_index_observation_m10.verification.json"),
]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def runtime_binding(data: dict[str, object]) -> dict[str, object]:
    for key in ("runtime_evidence", "runtime"):
        value = data.get(key)
        if isinstance(value, dict) and isinstance(value.get("path"), str) and \
                isinstance(value.get("sha256"), str):
            return value
    raise RuntimeError("manifest has no singular runtime binding")


def update_images(data: dict[str, object]) -> None:
    images = data.get("images")
    if isinstance(images, dict):
        for binding in images.values():
            if isinstance(binding, dict) and isinstance(binding.get("path"), str) and \
                    isinstance(binding.get("sha256"), str):
                path = ROOT / binding["path"]
                binding["sha256"] = sha256(path)
    image = data.get("image")
    if isinstance(image, dict) and isinstance(image.get("path"), str) and \
            isinstance(image.get("sha256"), str):
        image["sha256"] = sha256(ROOT / image["path"])


def main() -> None:
    if not FIXTURE.is_file():
        raise SystemExit(f"missing virtio fixture {FIXTURE}")
    log_path = ARTIFACTS / "phase78_nxe_modern_virtio_revalidation.log"
    with log_path.open("w") as aggregate:
        for build_name, manifest_name in PROFILES:
            build = ROOT / build_name
            manifest_path = ARTIFACTS / manifest_name
            data = json.loads(manifest_path.read_text())
            runtime = runtime_binding(data)
            transcript = ROOT / runtime["path"]
            aggregate.write(f"BEGIN {build_name} {manifest_name}\n")
            aggregate.flush()
            command = (
                "timeout 12s qemu-system-x86_64 -cpu max -nographic -serial mon:stdio "
                "-m size=1G -drive if=none,format=raw,file=../tests/artifacts/"
                "selinos_virtio_fixture.raw,id=selinosvd0 "
                "-device virtio-blk-pci,disable-legacy=on,drive=selinosvd0 "
                "-kernel images/kernel-x86_64-pc99 -initrd images/selinos-root-image-x86_64-pc99"
            )
            transcript.parent.mkdir(parents=True, exist_ok=True)
            result = subprocess.run(
                ["script", "-qefc", command, str(transcript)], cwd=build,
                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=False,
            )
            if result.returncode not in (0, 124):
                aggregate.write(f"FAIL {build_name} qemu_exit={result.returncode}\n")
                raise SystemExit(1)
            required = runtime.get("required_markers", [])
            output = transcript.read_text(errors="replace")
            missing = [marker for marker in required if marker not in output]
            if missing:
                aggregate.write(f"FAIL {build_name} missing={missing[0]}\n")
                raise SystemExit(1)
            update_images(data)
            runtime["sha256"] = sha256(transcript)
            manifest_path.write_text(json.dumps(data, indent=2) + "\n")
            aggregate.write(f"PASS {build_name} qemu_exit={result.returncode}\n")
            aggregate.flush()
        aggregate.write(f"MODERN_VIRTIO_REVALIDATION_PASS count={len(PROFILES)}\n")


if __name__ == "__main__":
    main()
