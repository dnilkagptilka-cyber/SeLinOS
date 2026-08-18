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
        (root / 'tests/artifacts/selinos_taskd_zeroed_context_m0.verification.json').read_text()
    )
    require(record['profile']['cmake_option'] == 'SeLinTaskdZeroedContextProbe=ON',
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

    for marker in ('SeLinTaskdZeroedContextProbe',
                   'SELINOS_TASKD_ZEROED_CONTEXT_PROBE',
                   'selinos-taskd-zeroed-context-m0'):
        require(marker in cmake, f'CMake marker: {marker}')
    for marker in ('CNODE_SLOT_BITS', 'TARGET_CNODE_SELF_SLOT',
                   'TARGET_NOTIFICATION_SLOT', 'TARGET_FRAME_SLOT',
                   'FIXED_IPC_BUFFER_VADDR', 'X86_64_CONTEXT_WORDS',
                   'X86_64_RFLAGS_WORD', 'X86_64_NORMALIZED_RFLAGS'):
        require('SELINOS_TASKD_ZEROED_CONTEXT_M0_' + marker in protocol,
                f'protocol marker: {marker}')

    begin = source.index('static bool start_taskd_zeroed_context_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    rollback = 'vka_free_object(vka, &rollback_frame);'
    map_page = 'sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,'
    copy_frame = 'SELINOS_TASKD_ZEROED_CONTEXT_M0_TARGET_FRAME_SLOT'
    configure = 'seL4_TCB_Configure('
    write = 'seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,'
    read = 'seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,'
    move = 'sel4utils_move_cap_to_process(&taskd,'
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 3,
            'expected self, notification, and frame CNode copies')
    require(bundle.index(rollback) < bundle.index(map_page) < bundle.index(copy_frame) <
            bundle.index(configure) < bundle.index(write) < bundle.index(read) <
            bundle.index(move), 'rollback/map/copy/configure/write/read/move order')
    require(bundle.count(write) == 1 and bundle.count(read) == 1,
            'expected exactly one whole-context write and read')
    require(bundle.count(move) == 3, 'expected final TCB/CNode/PML4 moves only')
    require('seL4_UserContext zero_context = {0};' in bundle and
            'X86_64_CONTEXT_WORDS' in bundle and
            'X86_64_RFLAGS_WORD' in bundle and
            'X86_64_NORMALIZED_RFLAGS' in bundle,
            'zero-request or normalized-RFLAGS contract absent')
    for marker in ('seL4_TCB_Resume', 'seL4_TCB_SetPriority',
                   'seL4_TCB_SetSchedParams', 'seL4_TCB_SetTLSBase',
                   'seL4_CNode_Mint', 'seL4_CNode_Move', 'clone', 'fork',
                   'pthread'):
        require(marker not in bundle + server + probe,
                f'forbidden operation: {marker}')
    require('target not invoked' in server and
            'SELINOS_TASKD_ZEROED_CONTEXT_M0_OWNED' in probe and
            'SELINOS_TASKD_ZEROED_CONTEXT_M0_REJECTED' in probe,
            'status-only ownership transaction missing')
    require('kernel-normalized `rflags=0x202`' in gate and
            'seL4_TCB_Resume' in gate and 'no Linux thread/process' in gate,
            'gate scope boundary missing')
    print('SeLinOS zero-request plus normalized-RFLAGS suspended task M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
