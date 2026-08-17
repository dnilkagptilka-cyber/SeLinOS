#!/usr/bin/env python3
"""Verify that the M15 probe differs from the M14 object only as declared."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
import tempfile


OLD_FSTAT_SETUP = (
    b"\x48\x89\xc7"              # mov %rax,%rdi
    b"\x4c\x89\xe6"              # mov %r12,%rsi
    b"\x48\xc7\xc0\x05\x00\x00\x00"  # mov $5,%rax
)
M15_WINDOW_AND_FSTAT_SETUP = (
    b"\x48\xc7\xc0\x48\x00\x00\x00"  # mov $72,%rax
    b"\x48\xc7\xc7\x06\x00\x00\x00"  # mov $6,%rdi
    b"\x48\xc7\xc6\x03\x00\x00\x00"  # mov $3,%rsi
    b"\x48\x31\xd2"                    # xor %rdx,%rdx
    b"\x0f\x05"                          # syscall
    b"\x48\x85\xc0"                    # test %rax,%rax
    b"\x0f\x85\x00\x00\x00\x00"      # jne failure (normalised)
    b"\x48\xc7\xc7\x06\x00\x00\x00"  # restore FD 6 for fstat
    b"\x4c\x89\xe6"                    # existing fstat buffer argument
    b"\x48\xc7\xc0\x05\x00\x00\x00"  # existing fstat syscall number
)


def dump_text(obj: pathlib.Path) -> bytes:
    with tempfile.TemporaryDirectory() as directory:
        output = pathlib.Path(directory) / "text.bin"
        subprocess.run(
            ["objcopy", "--dump-section", f".text={output}", str(obj)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        return output.read_bytes()


def normalise_near_conditional_displacements(text: bytes) -> bytes:
    result = bytearray(text)
    index = 0
    while index + 5 < len(result):
        if result[index] == 0x0F and 0x80 <= result[index + 1] <= 0x8F:
            result[index + 2:index + 6] = b"\x00\x00\x00\x00"
            index += 6
        else:
            index += 1
    return bytes(result)


def replace_exactly_once(text: bytes, old: bytes, new: bytes) -> bytes:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"expected one declared M10 fstat setup, found {count}")
    return text.replace(old, new, 1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline", type=pathlib.Path)
    parser.add_argument("candidate", type=pathlib.Path)
    args = parser.parse_args()

    baseline = normalise_near_conditional_displacements(dump_text(args.baseline))
    candidate = normalise_near_conditional_displacements(dump_text(args.candidate))
    expected = replace_exactly_once(baseline, OLD_FSTAT_SETUP, M15_WINDOW_AND_FSTAT_SETUP)

    if candidate != expected:
        print("M15 probe object guard failed: changes extend beyond the declared window.", file=sys.stderr)
        return 1

    print("M15 probe object guard passed: only the declared F_GETFL window changed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
