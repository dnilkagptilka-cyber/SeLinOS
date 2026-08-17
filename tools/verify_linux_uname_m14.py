#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from __future__ import annotations
import hashlib, json, sys
from pathlib import Path

def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()

def need(value: bool, message: str) -> None:
    if not value: raise RuntimeError(message)

def bind(root: Path, item: dict, label: str) -> Path:
    p = root / item['path']; need(p.is_file(), f'missing {label}'); need(digest(p) == item['sha256'], f'{label} SHA-256 mismatch'); return p

def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rec = json.loads((root / 'tests/artifacts/selinos_linux_uname_m14.verification.json').read_text())
    need(rec['schema'] == 1 and rec['platform']['linux_kernel_present'] is False, 'invalid M14 platform evidence')
    bind(root, rec['image'], 'image')
    dispatcher = bind(root, rec['implementation']['root_dispatcher'], 'dispatcher').read_text()
    probe = bind(root, rec['implementation']['isolated_probe'], 'probe').read_text()
    log = bind(root, rec['runtime_evidence'], 'runtime log').read_text(errors='replace')
    for s in ('SELINOS_LINUX_UNAME 63u', 'SELINOS_LINUX_UTSNAME_FIELD_BYTES 65u', 'SELINOS_LINUX_UTSNAME_BYTES', 'copy_linux_bytes_to_user', 'fixed 390-byte x86_64 utsname-layout record mediated'):
        need(s in dispatcher, f'missing dispatcher control: {s}')
    for s in ('SELINOS_LINUX_SYS_UNAME 63ul', '"mov $63, %%rax\\n"', '"cmpb $\'L\', (%%r12)\\n"', '"cmpb $\'l\', 325(%%r12)\\n"'):
        need(s in probe, f'missing probe control: {s}')
    for s in rec['runtime_evidence']['required_markers']: need(s in log, f'missing marker: {s}')
    for s in rec['runtime_evidence']['forbidden_markers']: need(s not in log, f'forbidden marker: {s}')
    claims = ' '.join(rec['not_claimed'])
    for s in ('host or QEMU', 'UTS namespaces', 'dpkg or apt'): need(s in claims, f'missing non-claim: {s}')
    print('SeLinOS Linux uname M14 evidence verified.')
    return 0
if __name__ == '__main__':
    try: raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as e:
        print(f'verification failed: {e}', file=sys.stderr); raise SystemExit(1)
