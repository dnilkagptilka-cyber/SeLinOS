#!/usr/bin/env python3
"""Verify SeLinOS root-only post-notification zero-index M10 evidence."""
from __future__ import annotations
import hashlib, json, sys
from pathlib import Path


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()


def req(ok: bool, msg: str) -> None:
    if not ok:
        raise RuntimeError(msg)


def bound(root: Path, item: dict, name: str) -> Path:
    p = root / item['path']
    req(p.is_file(), f'missing {name}')
    req(digest(p) == item['sha256'], f'{name} SHA-256 mismatch')
    return p


def fun(src: str, sig: str) -> str:
    s = src.index(sig); b = src.index('{', s); d = 0
    for i in range(b, len(src)):
        d += (src[i] == '{') - (src[i] == '}')
        if d == 0:
            return src[s:i + 1]
    raise RuntimeError('unterminated helper')


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    e = json.loads((root / 'tests/artifacts/selinos_virtio_zero_index_observation_m10.verification.json').read_text())
    req(e['schema'] == 1, 'unexpected schema')
    p = e['profile']
    for k, v in {'build_directory':'build-virtio-zero-index-observation-probe','cmake_option':'SeLinRootVirtioBlkZeroIndexObservationProbe=ON','device_identity':'1af4:1042','queue_index':0,'programmed_queue_size':1,'notification_writes':1,'notification_value':0,'final_device_status':0,'queue_enable_after_reset':0}.items():
        req(p[k] == v, f'wrong profile {k}')
    for name, item in e['images'].items():
        req(bound(root, item, name).stat().st_size > 0, f'empty {name}')
    src = {k: bound(root, v, k).read_text(errors='replace') for k, v in e['implementation'].items()}
    fixture = bound(root, e['fixture'], 'fixture').read_bytes()
    req(len(fixture) == 8 * 1024 * 1024 and set(fixture) <= {0}, 'fixture changed')
    log = bound(root, e['runtime_evidence'], 'runtime log').read_text(errors='replace')
    for marker in e['runtime_evidence']['required_markers']:
        req(marker in log, f'missing runtime marker: {marker}')
    for marker in e['runtime_evidence']['forbidden_markers']:
        req(marker not in log, f'forbidden runtime marker: {marker}')
    before = log.rsplit('fixture_before=', 1)[1].splitlines()[0]
    after = log.rsplit('fixture_after=', 1)[1].splitlines()[0]
    req(before == after == e['fixture']['sha256'], 'fixture hash mismatch in trace')

    for x in ('SeLinRootVirtioBlkZeroIndexObservationProbe','SELINOS_ROOT_VIRTIO_BLK_ZERO_INDEX_OBSERVATION_PROBE','Enable root-only virtio post-notification zero-index observation proof'):
        req(x in src['build_gate'], f'missing CMake gate {x}')
    for x in ('CONFIG_SELINOS_ROOT_VIRTIO_BLK_ZERO_INDEX_OBSERVATION_PROBE','immediate avail/used zero-index observation passed','no completion, IRQ, DMA claim, containment claim or block I/O'):
        req(x in src['root_wiring'], f'missing root wiring {x}')
    for x in ('avail_flags_before','avail_index_before','used_flags_before','used_index_before','avail_flags_after','avail_index_after','used_flags_after','used_index_after'):
        req(x in src['pci_header'], f'missing stage field {x}')

    pci = src['pci_helper']
    for x in ('VIRTIO_SPLIT_DRIVER_OFFSET 16u','VIRTIO_SPLIT_DEVICE_OFFSET 24u','VIRTIO_SPLIT_FLAGS_OFFSET   0u','VIRTIO_SPLIT_INDEX_OFFSET   2u'):
        req(x in pci, f'missing split-ring offset {x}')
    h = fun(pci, 'bool selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset')
    for x in ('layout[index] = 0u;','stage->avail_flags_before = *avail_flags;','stage->avail_index_before = *avail_index;','stage->used_flags_before = *used_flags;','stage->used_index_before = *used_index;','*notification = VIRTIO_COMMON_QUEUE_ZERO;','stage->avail_flags_after = *avail_flags;','stage->avail_index_after = *avail_index;','stage->used_flags_after = *used_flags;','stage->used_index_after = *used_index;','*queue_enable = 1u;','stage->queue_enable_after_reset = *queue_enable;'):
        req(x in h, f'missing helper control {x}')
    order = [h.index(x) for x in ('layout[index] = 0u;','stage->avail_index_before = *avail_index;','*queue_enable = 1u;','*notification = VIRTIO_COMMON_QUEUE_ZERO;','stage->avail_index_after = *avail_index;')]
    req(order == sorted(order), 'M10 sample/notification sequence unordered')
    req(h.count('    *notification = VIRTIO_COMMON_QUEUE_ZERO;') == 1, 'must notify exactly once')
    for x in ('    *avail_index =','    *used_index =','layout[16u] =','layout[24u] =','seL4_IRQHandler','seL4_IRQControl','seL4_X86_IOSpace','vka_alloc_dma','block request'):
        req(x not in h, f'forbidden M10 control {x}')

    design = src['phase_design_gate']
    for x in ('Status: verified, bounded M10 proof.','QEMU trace observation only','does not establish that the device did not DMA','Explicit non-claims','dpkg` or `apt`'):
        req(x in design, f'missing design boundary {x}')
    claims = e['validated_contract']
    for field, word in [('samples','all four before'),('notification','once'),('queue_state','queue_size=1'),('authority','DMA/containment claim')]:
        req(word in claims[field], f'missing claim {field}')
    no = ' '.join(e['not_claimed'])
    for x in ('no DMA','device completion','block request','persistent VFS','dpkg or apt'):
        req(x in no, f'missing non-claim {x}')
    print('SeLinOS virtio-blk root-only post-notification zero-index observation M10 evidence verified.')
    return 0

if __name__ == '__main__':
    try: raise SystemExit(main())
    except (RuntimeError, KeyError, ValueError) as ex:
        print(f'verification failed: {ex}', file=sys.stderr); raise SystemExit(1)
