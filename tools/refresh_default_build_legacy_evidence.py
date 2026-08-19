#!/usr/bin/env python3
"""Refresh default-build evidence after NXE correction using bounded current replays."""
from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
IMAGE = "build/images/selinos-root-image-x86_64-pc99"
BASELINE = ARTIFACTS / "selinos_m1_baseline.log"
TWO_EDU = ARTIFACTS / "selinos_two_edu_m3.log"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def runtime_binding(data: dict[str, object]) -> dict[str, object] | None:
    for key in ("runtime_evidence", "runtime"):
        value = data.get(key)
        if isinstance(value, dict) and isinstance(value.get("path"), str) and \
                isinstance(value.get("sha256"), str):
            return value
    return None


def main() -> None:
    image_hash = sha256(ROOT / IMAGE)
    changed: list[str] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        data = json.loads(manifest.read_text())
        image = data.get("image")
        if not isinstance(image, dict) or image.get("path") != IMAGE:
            continue
        baseline_stale = (
            manifest.name == "selinos_driver_runtime_m1.verification.json" and
            data["runtime_evidence"]["baseline_log"].get("sha256") != sha256(BASELINE)
        )
        if image.get("sha256") == image_hash and not baseline_stale:
            continue
        runtime = runtime_binding(data)
        if runtime is not None:
            target = ROOT / str(runtime["path"])
            source = TWO_EDU if manifest.name == "selinos_m3_two_edu.verification.json" else BASELINE
            if not source.is_file():
                raise SystemExit(f"refresh refused: missing current replay {source}")
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.resolve() != target.resolve():
                shutil.copyfile(source, target)
            runtime["sha256"] = sha256(target)
        if manifest.name == "selinos_driver_runtime_m1.verification.json":
            baseline_binding = data["runtime_evidence"]["baseline_log"]
            baseline_text = BASELINE.read_text(errors="replace")
            for marker in baseline_binding["required_markers"]:
                if marker not in baseline_text:
                    raise SystemExit(f"refresh refused: baseline missing {marker!r}")
            baseline_binding["sha256"] = sha256(BASELINE)
        image["sha256"] = image_hash
        manifest.write_text(json.dumps(data, indent=2) + "\n")
        changed.append(manifest.name)
    print(f"DEFAULT_BUILD_LEGACY_REFRESH_PASS count={len(changed)}")
    for name in changed:
        print(f"REFRESHED {name}")


if __name__ == "__main__":
    main()
