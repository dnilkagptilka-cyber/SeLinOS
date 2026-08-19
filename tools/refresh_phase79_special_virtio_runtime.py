#!/usr/bin/env python3
"""Refresh only Phase 79 replay-backed Virtio M5-M10 runtime bindings."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STEMS = (
    "selinos_virtio_blk_queue_zero_m5",
    "selinos_virtio_queue_layout_m6",
    "selinos_virtio_queue_enable_m7",
    "selinos_virtio_notification_observation_m8",
    "selinos_virtio_zero_descriptor_notification_m9",
    "selinos_virtio_zero_index_observation_m10",
)
HASH_SERIALIZED = set(STEMS[1:])


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    for stem in STEMS:
        manifest_path = ROOT / "tests/artifacts" / f"{stem}.verification.json"
        manifest = json.loads(manifest_path.read_text())
        runtime = manifest["runtime_evidence"]
        log_path = ROOT / runtime["path"]
        text = log_path.read_text(errors="replace")
        for marker in runtime["required_markers"]:
            require(marker in text, f"{stem}: missing marker {marker!r}")
        for marker in runtime["forbidden_markers"]:
            require(marker not in text, f"{stem}: forbidden marker {marker!r}")
        fixture = manifest.get("fixture")
        if fixture is not None:
            fixture_path = ROOT / fixture["path"]
            fixture_hash = sha256(fixture_path)
            require(fixture_hash == fixture["sha256"], f"{stem}: immutable fixture changed")
            if stem in HASH_SERIALIZED:
                require(f"fixture_before={fixture_hash}" in text,
                        f"{stem}: missing fixture_before record")
                require(f"fixture_after={fixture_hash}" in text,
                        f"{stem}: missing fixture_after record")
        runtime["sha256"] = sha256(log_path)
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
        print(f"REFRESHED {manifest_path.name}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"refresh failed: {error}")
        raise SystemExit(1)
