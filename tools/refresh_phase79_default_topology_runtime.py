#!/usr/bin/env python3
"""Atomically refresh default-image runtime traces from current QEMU topology runs."""
from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
DEFAULT_IMAGE = "build/images/selinos-root-image-x86_64-pc99"
TRACES = {
    "none": ARTIFACTS / "selinos_m1_baseline.log",
    "one": ARTIFACTS / "selinos_phase79_one_edu_current_image.log",
    "two": ARTIFACTS / "selinos_phase79_two_edu_current_image.log",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bindings(value: object) -> list[dict[str, object]]:
    if isinstance(value, dict):
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            return [value] if str(value["path"]).endswith(".log") else []
        result: list[dict[str, object]] = []
        for child in value.values():
            result.extend(bindings(child))
        return result
    if isinstance(value, list):
        result: list[dict[str, object]] = []
        for child in value:
            result.extend(bindings(child))
        return result
    return []


def topology(required: list[str]) -> str:
    joined = "\n".join(required).lower()
    if "multiple qemu edu" in joined or "independent bundles" in joined:
        return "two"
    if "not present" in joined or " edu absent" in joined or " dormant" in joined:
        return "none"
    if "edu" in joined:
        return "one"
    return "none"


def main() -> int:
    trace_text = {name: path.read_text(errors="replace") for name, path in TRACES.items()}
    pending: list[tuple[Path, dict[str, object], list[tuple[dict[str, object], Path]]]] = []
    failures: list[str] = []
    for manifest_path in sorted(ARTIFACTS.glob("*.verification.json")):
        data = json.loads(manifest_path.read_text())
        image = data.get("image")
        if not isinstance(image, dict) or image.get("path") != DEFAULT_IMAGE:
            continue
        updates: list[tuple[dict[str, object], Path]] = []
        for binding in bindings(data):
            required_markers = binding.get("required_markers", [])
            required_once = binding.get("required_once", [])
            required_twice = binding.get("required_twice", [])
            forbidden = binding.get("forbidden_markers", [])
            if not all(isinstance(value, list) for value in
                       (required_markers, required_once, required_twice, forbidden)):
                continue
            required = [str(marker) for marker in required_markers + required_once + required_twice]
            kind = topology(required)
            source = TRACES[kind]
            text = trace_text[kind]
            for marker in required_markers + required_once:
                if str(marker) not in text:
                    failures.append(f"{manifest_path.name}: {kind} missing {marker!r}")
            for marker in required_twice:
                if text.count(str(marker)) < 2:
                    failures.append(f"{manifest_path.name}: {kind} lacks two occurrences of {marker!r}")
            for marker in forbidden:
                if str(marker) in text:
                    failures.append(f"{manifest_path.name}: {kind} forbidden {marker!r}")
            updates.append((binding, source))
        if updates:
            pending.append((manifest_path, data, updates))
    if failures:
        print(f"DEFAULT_TOPOLOGY_RUNTIME_REFRESH_BLOCKED gaps={len(failures)}")
        for failure in failures:
            print(f"GAP {failure}")
        return 1
    for manifest_path, data, updates in pending:
        for binding, source in updates:
            target = ROOT / str(binding["path"])
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.resolve() != target.resolve():
                shutil.copyfile(source, target)
            binding["sha256"] = sha256(target)
        manifest_path.write_text(json.dumps(data, indent=2) + "\n")
    print(f"DEFAULT_TOPOLOGY_RUNTIME_REFRESH_PASS count={len(pending)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
