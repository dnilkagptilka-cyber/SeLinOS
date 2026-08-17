#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    root = Path(__file__).resolve().parent.parent
    record = json.loads(
        (root / 'tests/artifacts/selinos_taskd_ipc_buffer_m0.verification.json').read_text()
    )
    require(record['profile']['cmake_option'] == 'SeLinTaskdIpcBufferProbe=ON',
            'incorrect CMake proof gate')
    for group in ('images', 'implementation'):
        for name, item in record[group].items():
            path = root / item['path']
            require(path.is_file() and digest(path) == item['sha256'],
                    f'{group}:{name} hash')
    runtime = root / record['runtime']['path']
    require(digest(runtime) == record['runtime']['sha256'], 'runtime transcript hash')
    transcript = runtime.read_text(errors='replace')
    for marker in record['runtime']['required_markers']:
        require(marker in transcript, f'missing marker: {marker}')

    source = (root / record['implementation']['root']['path']).read_text()
    cmake = (root / record['implementation']['cmake']['path']).read_text()
    protocol = (root / record['implementation']['protocol']['path']).read_text()
    server = (root / record['implementation']['server']['path']).read_text()
    probe = (root / record['implementation']['probe']['path']).read_text()
    gate = (root / record['implementation']['gate']['path']).read_text()

    for marker in ('SeLinTaskdIpcBufferProbe',
                   'SELINOS_TASKD_IPC_BUFFER_PROBE',
                   'selinos-taskd-ipc-buffer-m0'):
        require(marker in cmake, f'CMake marker: {marker}')
    for marker in ('CNODE_SLOT_BITS', 'TARGET_CNODE_SELF_SLOT',
                   'TARGET_NOTIFICATION_SLOT', 'TARGET_FRAME_SLOT',
                   'FIXED_VADDR'):
        require('SELINOS_TASKD_IPC_BUFFER_M0_' + marker in protocol,
                f'protocol marker: {marker}')

    begin = source.index('static bool start_taskd_ipc_buffer_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    rollback = 'vka_free_object(vka, &rollback_frame);'
    map_page = 'sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,'
    copy_frame = 'SELINOS_TASKD_IPC_BUFFER_M0_TARGET_FRAME_SLOT'
    configure = 'seL4_TCB_Configure('
    move = 'sel4utils_move_cap_to_process(&taskd,'
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 3,
            'expected self, notification, and frame CNode copies')
    require(bundle.index(rollback) < bundle.index(map_page) < bundle.index(copy_frame) <
            bundle.index(configure) < bundle.index(move),
            'rollback/map/copy/configure/move order')
    require(bundle.count(move) == 3, 'expected final TCB/CNode/PML4 moves only')
    require('SELINOS_TASKD_IPC_BUFFER_M0_FIXED_VADDR' in bundle and
            'vka_alloc_frame(vka, seL4_PageBits, &target_frame)' in bundle,
            'frame allocation or fixed IPC mapping absent')
    forbidden = ('seL4_TCB_WriteRegisters', 'seL4_TCB_Resume',
                 'seL4_TCB_SetIPCBuffer', 'clone', 'fork', 'pthread')
    for marker in forbidden:
        require(marker not in bundle + server + probe, f'forbidden operation: {marker}')
    require('target not invoked' in server and
            'SELINOS_TASKD_IPC_BUFFER_M0_OWNED' in probe and
            'SELINOS_TASKD_IPC_BUFFER_M0_REJECTED' in probe,
            'status-only ownership transaction missing')
    require('target execution is out of scope' in gate.lower() and
            '512-byte-aligned' in gate,
            'gate boundary missing')
    print('SeLinOS isolated IPC-buffer mapped suspended task M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
