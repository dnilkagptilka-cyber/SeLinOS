#!/usr/bin/env python3
"""Generate the deterministic non-executing Phase 64 SSIM-v1 fixture and ledger."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "tests" / "artifacts" / "sealed_static_image_m0"
IMAGE_PATH = OUT_DIR / "valid.ssim"
LEDGER_PATH = OUT_DIR / "valid.ledger.json"
IMAGE_BYTES = 4096
HEADER_BYTES = 64
SEGMENT_BYTES = 32
SOURCE_BYTES = HEADER_BYTES + SEGMENT_BYTES + 3
ENTRY_VADDR = 0x60000000
DIGEST_OFFSET = 32
DIGEST_BYTES = 32
PAYLOAD_OFFSET = HEADER_BYTES + SEGMENT_BYTES
PAYLOAD = bytes([0x90, 0x0F, 0x0B])


def put_u16(image: bytearray, offset: int, value: int) -> None:
    image[offset : offset + 2] = value.to_bytes(2, "little")


def put_u32(image: bytearray, offset: int, value: int) -> None:
    image[offset : offset + 4] = value.to_bytes(4, "little")


def put_u64(image: bytearray, offset: int, value: int) -> None:
    image[offset : offset + 8] = value.to_bytes(8, "little")


def main() -> None:
    image = bytearray(IMAGE_BYTES)
    image[0:8] = b"SELINS64"
    put_u16(image, 8, 1)
    put_u16(image, 10, HEADER_BYTES)
    put_u16(image, 12, 1)
    put_u32(image, 16, HEADER_BYTES)
    put_u32(image, 20, SOURCE_BYTES)
    put_u64(image, 24, ENTRY_VADDR)
    put_u64(image, 64, ENTRY_VADDR)
    put_u32(image, 72, PAYLOAD_OFFSET)
    put_u32(image, 76, len(PAYLOAD))
    put_u32(image, 80, len(PAYLOAD))
    put_u16(image, 84, 0x5)
    image[PAYLOAD_OFFSET : PAYLOAD_OFFSET + len(PAYLOAD)] = PAYLOAD
    source = bytearray(image[:SOURCE_BYTES])
    source[DIGEST_OFFSET : DIGEST_OFFSET + DIGEST_BYTES] = bytes(DIGEST_BYTES)
    source_sha256 = hashlib.sha256(source).hexdigest()
    image[DIGEST_OFFSET : DIGEST_OFFSET + DIGEST_BYTES] = bytes.fromhex(source_sha256)
    ledger = {
        "format": "SeLin Sealed Image version 1",
        "source_bytes": SOURCE_BYTES,
        "entry_vaddr": f"0x{ENTRY_VADDR:016x}",
        "payload_offset": PAYLOAD_OFFSET,
        "payload_bytes": len(PAYLOAD),
        "permissions": "R|X",
        "wire_sha256": hashlib.sha256(image).hexdigest(),
        "source_sha256": source_sha256,
        "payload_sha256": hashlib.sha256(PAYLOAD).hexdigest(),
    }
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    IMAGE_PATH.write_bytes(image)
    LEDGER_PATH.write_text(json.dumps(ledger, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"generated {IMAGE_PATH.relative_to(ROOT)}")
    print(json.dumps(ledger, sort_keys=True))


if __name__ == "__main__":
    main()
