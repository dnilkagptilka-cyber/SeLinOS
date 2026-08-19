#!/usr/bin/env python3
"""Replay every evidence profile whose rebuilt image hash differs from its manifest.

This tool intentionally updates only runtime transcripts.  A separate, reviewed
binding step is required before any manifest SHA can change.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "tests/artifacts"
AGGREGATE = ARTIFACTS / "phase78_nxe_affected_profile_replays.log"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def runtime_binding(data: dict[str, object]) -> dict[str, object] | None:
    for key in ("runtime", "runtime_evidence"):
        value = data.get(key)
        if isinstance(value, dict) and isinstance(value.get("path"), str):
            return value
    return None


def all_bindings(value: object) -> list[dict[str, object]]:
    if isinstance(value, dict):
        if isinstance(value.get("path"), str) and isinstance(value.get("sha256"), str):
            return [value]
        found: list[dict[str, object]] = []
        for item in value.values():
            found.extend(all_bindings(item))
        return found
    if isinstance(value, list):
        found: list[dict[str, object]] = []
        for item in value:
            found.extend(all_bindings(item))
        return found
    return []


def image_bindings(data: dict[str, object]) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    for binding in all_bindings(data):
        parts = Path(str(binding["path"])).parts
        if len(parts) >= 3 and parts[0].startswith("build-") and parts[1] == "images":
            result.append(binding)
    return result


def main() -> None:
    grouped: dict[tuple[Path, Path], list[str]] = defaultdict(list)
    skipped: list[str] = []
    for manifest in sorted(ARTIFACTS.glob("*.verification.json")):
        try:
            data = json.loads(manifest.read_text())
        except json.JSONDecodeError:
            continue
        images = image_bindings(data)
        if not images:
            continue
        stale = False
        build_dir: Path | None = None
        for item in images:
            path = ROOT / str(item["path"])
            if not path.is_file() or sha256(path) != item["sha256"]:
                stale = True
            parts = Path(str(item["path"])).parts
            if len(parts) >= 3 and parts[0].startswith("build-") and parts[1] == "images":
                build_dir = ROOT / parts[0]
        if not stale:
            continue
        runtime = runtime_binding(data)
        if build_dir is None or runtime is None:
            skipped.append(manifest.name)
            continue
        grouped[(build_dir, ROOT / str(runtime["path"]))].append(manifest.name)

    with AGGREGATE.open("w") as log:
        log.write(f"AFFECTED_PROFILE_REPLAY_COUNT={len(grouped)}\n")
        for (build_dir, runtime), manifests in sorted(grouped.items()):
            name = build_dir.name
            log.write(f"BEGIN {name} manifests={','.join(manifests)}\n")
            log.flush()
            command = (
                "timeout 12s qemu-system-x86_64 -cpu max -nographic "
                "-serial mon:stdio -m size=1G -kernel images/kernel-x86_64-pc99 "
                "-initrd images/selinos-root-image-x86_64-pc99"
            )
            runtime.parent.mkdir(parents=True, exist_ok=True)
            result = subprocess.run(
                ["script", "-qefc", command, str(runtime)], cwd=build_dir,
                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=False,
            )
            if result.returncode not in (0, 124):
                log.write(f"FAIL {name} qemu_exit={result.returncode}\n")
                log.flush()
                raise SystemExit(1)
            log.write(f"PASS {name} qemu_exit={result.returncode}\n")
            log.flush()
        for manifest in skipped:
            log.write(f"SKIP {manifest} no runnable profile/runtime mapping\n")
        log.write(f"AFFECTED_PROFILE_REPLAY_PASS count={len(grouped)} skipped={len(skipped)}\n")


if __name__ == "__main__":
    main()
