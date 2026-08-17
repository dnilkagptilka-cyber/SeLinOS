#!/usr/bin/env python3
"""Generate a version-pinned SeLinOS KABI manifest from Linux build artifacts."""

from __future__ import annotations

import hashlib
import json
import pathlib
import subprocess
import sys
from collections import Counter


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_uts_release(path: pathlib.Path) -> str:
    text = path.read_text(encoding="utf-8").strip()
    prefix = '#define UTS_RELEASE "'
    if not text.startswith(prefix) or not text.endswith('"'):
        raise ValueError(f"Unexpected utsrelease.h content: {text!r}")
    return text[len(prefix):-1]


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: generate_kabi_manifest.py <linux-build-dir> <source-tarball> <output-json>", file=sys.stderr)
        return 2

    build_dir = pathlib.Path(sys.argv[1]).resolve()
    source_tarball = pathlib.Path(sys.argv[2]).resolve()
    output = pathlib.Path(sys.argv[3]).resolve()
    symvers = build_dir / "vmlinux.symvers"
    config = build_dir / ".config"
    uts_release = build_dir / "include/generated/utsrelease.h"
    system_map = build_dir / "System.map"
    vmlinux = build_dir / "vmlinux"
    modules_builtin_modinfo = build_dir / "modules.builtin.modinfo"

    for required in (source_tarball, symvers, config, uts_release, system_map, vmlinux, modules_builtin_modinfo):
        if not required.is_file():
            raise FileNotFoundError(required)

    exports = []
    kind_counts: Counter[str] = Counter()
    for line_number, raw in enumerate(symvers.read_text(encoding="utf-8").splitlines(), 1):
        if not raw:
            continue
        fields = raw.split("\t")
        if len(fields) < 4:
            raise ValueError(f"Malformed symvers record at line {line_number}: {raw!r}")
        crc, symbol, owner, export_kind, *rest = fields
        namespace = rest[0] if rest else ""
        record = {
            "crc": crc,
            "symbol": symbol,
            "owner": owner,
            "export_kind": export_kind,
            "namespace": namespace,
        }
        exports.append(record)
        kind_counts[export_kind] += 1

    compiler = subprocess.check_output(["gcc", "--version"], text=True).splitlines()[0]
    manifest = {
        "schema": "selinos.kabi-manifest/v1",
        "baseline": "SELINOS_LINUX_BASELINE_6_18_44",
        "architecture": "x86_64",
        "kernel_release": read_uts_release(uts_release),
        "toolchain": {"compiler": compiler},
        "artifacts": {
            "linux_source_tarball": {"path": source_tarball.name, "sha256": sha256(source_tarball)},
            "config": {"path": ".config", "sha256": sha256(config)},
            "vmlinux": {"path": "vmlinux", "sha256": sha256(vmlinux)},
            "system_map": {"path": "System.map", "sha256": sha256(system_map)},
            "vmlinux_symvers": {"path": "vmlinux.symvers", "sha256": sha256(symvers)},
            "modules_builtin_modinfo": {"path": "modules.builtin.modinfo", "sha256": sha256(modules_builtin_modinfo)},
            "uts_release_header": {"path": "include/generated/utsrelease.h", "sha256": sha256(uts_release)},
        },
        "exports": exports,
        "summary": {
            "export_count": len(exports),
            "export_kinds": dict(sorted(kind_counts.items())),
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote {output} with {len(exports)} exports")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
