#!/usr/bin/env python3
"""Verify an ELF64 Linux module against a SeLinOS version-pinned KABI manifest."""

from __future__ import annotations

import json
import pathlib
import struct
import sys

ELF_HEADER = struct.Struct("<16sHHIQQQIHHHHHH")
SECTION_HEADER = struct.Struct("<IIQQQQIIQQ")
SYMBOL = struct.Struct("<IBBHQQ")
VERSION_RECORD = struct.Struct("<Q56s")

SHT_SYMTAB = 2
SHN_UNDEF = 0
EM_X86_64 = 62
ET_REL = 1


def bounded_c_string(data: bytes, start: int) -> str:
    end = data.find(b"\0", start)
    if end == -1:
        raise ValueError("unterminated string")
    return data[start:end].decode("ascii", errors="strict")


def section_name(table: bytes, offset: int) -> str:
    if offset >= len(table):
        raise ValueError("section name out of range")
    return bounded_c_string(table, offset)


def parse_module(image: bytes) -> dict:
    if len(image) < ELF_HEADER.size:
        raise ValueError("truncated ELF header")
    ident, etype, machine, _, _, _, shoff, _, _, _, _, shentsize, shnum, shstrndx = ELF_HEADER.unpack_from(image)
    if ident[:4] != b"\x7fELF" or ident[4] != 2 or ident[5] != 1:
        raise ValueError("expected ELF64 little-endian module")
    if etype != ET_REL or machine != EM_X86_64:
        raise ValueError("expected ET_REL x86_64 module")
    if shentsize != SECTION_HEADER.size or shstrndx >= shnum:
        raise ValueError("invalid section headers")
    if shoff + shnum * shentsize > len(image):
        raise ValueError("truncated section table")

    sections = [SECTION_HEADER.unpack_from(image, shoff + index * shentsize) for index in range(shnum)]
    name_section = sections[shstrndx]
    _, _, _, _, names_offset, names_size, _, _, _, _ = name_section
    names = image[names_offset:names_offset + names_size]
    if len(names) != names_size:
        raise ValueError("truncated section-name table")

    named = {}
    for header in sections:
        name_offset, _, _, _, offset, size, _, _, _, _ = header
        if offset + size > len(image):
            raise ValueError("truncated section body")
        named[section_name(names, name_offset)] = header

    modinfo_header = named.get(".modinfo")
    if modinfo_header is None:
        raise ValueError("missing .modinfo")
    _, _, _, _, offset, size, _, _, _, _ = modinfo_header
    modinfo = {}
    for entry in image[offset:offset + size].split(b"\0"):
        if b"=" in entry:
            key, value = entry.split(b"=", 1)
            modinfo[key.decode("ascii")] = value.decode("ascii")

    undefined = set()
    for header in sections:
        _, section_type, _, _, offset, size, link, _, _, entsize = header
        if section_type != SHT_SYMTAB:
            continue
        if entsize != SYMBOL.size or size % entsize:
            raise ValueError("invalid symbol table")
        str_header = sections[link]
        _, _, _, _, str_offset, str_size, _, _, _, _ = str_header
        strings = image[str_offset:str_offset + str_size]
        for position in range(0, size, entsize):
            st_name, _, _, st_shndx, _, _ = SYMBOL.unpack_from(image, offset + position)
            if st_shndx == SHN_UNDEF and st_name:
                undefined.add(bounded_c_string(strings, st_name))

    versions = {}
    versions_header = named.get("__versions")
    if versions_header is not None:
        _, _, _, _, offset, size, _, _, _, _ = versions_header
        if size % VERSION_RECORD.size:
            raise ValueError("invalid __versions section")
        for position in range(0, size, VERSION_RECORD.size):
            crc, raw_name = VERSION_RECORD.unpack_from(image, offset + position)
            name = raw_name.split(b"\0", 1)[0].decode("ascii")
            versions[name] = f"0x{crc:08x}"

    return {
        "modinfo": modinfo,
        "undefined_symbols": sorted(undefined),
        "version_records": versions,
        "relocation_section_count": sum(1 for header in sections if header[1] == 4),
    }


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: verify_kabi_module.py <module.ko> <manifest.json> <report.json>", file=sys.stderr)
        return 2
    module_path = pathlib.Path(sys.argv[1])
    manifest_path = pathlib.Path(sys.argv[2])
    report_path = pathlib.Path(sys.argv[3])
    parsed = parse_module(module_path.read_bytes())
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    exports = {entry["symbol"]: entry["crc"].lower() for entry in manifest["exports"]}

    vermagic = parsed["modinfo"].get("vermagic", "")
    baseline = manifest["kernel_release"]
    unknown_undefined = [symbol for symbol in parsed["undefined_symbols"] if symbol not in exports]
    version_mismatches = {
        symbol: {"module": crc, "manifest": exports.get(symbol)}
        for symbol, crc in parsed["version_records"].items()
        if exports.get(symbol, "").lower() != crc.lower()
    }
    report = {
        "schema": "selinos.kabi-module-verification/v1",
        "module": module_path.name,
        "baseline": manifest["baseline"],
        "vermagic": vermagic,
        "vermagic_matches_baseline": vermagic.startswith(baseline),
        "module_name": parsed["modinfo"].get("name", ""),
        "license": parsed["modinfo"].get("license", ""),
        "undefined_symbols": parsed["undefined_symbols"],
        "unknown_undefined_symbols": unknown_undefined,
        "version_records": parsed["version_records"],
        "version_mismatches": version_mismatches,
        "relocation_section_count": parsed["relocation_section_count"],
    }
    report["accepted_for_next_loader_stage"] = (
        report["vermagic_matches_baseline"]
        and bool(report["module_name"])
        and bool(report["license"])
        and not unknown_undefined
        and not version_mismatches
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "module": report["module"],
        "accepted_for_next_loader_stage": report["accepted_for_next_loader_stage"],
        "undefined_symbols": len(report["undefined_symbols"]),
        "version_records": len(report["version_records"]),
        "version_mismatches": len(version_mismatches),
    }, sort_keys=True))
    return 0 if report["accepted_for_next_loader_stage"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
