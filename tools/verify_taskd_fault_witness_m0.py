#!/usr/bin/env python3
import hashlib
import json
import re
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
        (root / 'tests/artifacts/selinos_taskd_fault_witness_m0.verification.json').read_text()
    )
    require(record['profile']['cmake_option'] == 'SeLinTaskdFaultWitnessProbe=ON',
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

    for marker in ('SeLinTaskdFaultWitnessProbe',
                   'SELINOS_TASKD_FAULT_WITNESS_PROBE',
                   'selinos-taskd-fault-witness-m0'):
        require(marker in cmake, f'CMake marker: {marker}')
    for marker in ('TARGET_CNODE_SELF_SLOT', 'TARGET_NOTIFICATION_SLOT',
                   'TARGET_FRAME_SLOT', 'TARGET_FAULT_ENDPOINT_SLOT',
                   'FAULT_BADGE', 'FIXED_IPC_BUFFER_VADDR',
                   'X86_64_CONTEXT_WORDS', 'X86_64_RFLAGS_WORD',
                   'X86_64_NORMALIZED_RFLAGS'):
        require('SELINOS_TASKD_FAULT_WITNESS_M0_' + marker in protocol,
                f'protocol marker: {marker}')

    begin = source.index('static bool start_taskd_fault_witness_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    rollback = 'vka_free_object(vka, &rollback_frame);'
    map_page = 'sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,'
    configure = 'seL4_TCB_Configure(owned_tcb.cptr,'
    write = 'seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,'
    read = 'seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,'
    resume = 'seL4_TCB_Resume(owned_tcb.cptr)'
    receive = 'fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);'
    move = 'sel4utils_move_cap_to_process(&taskd,'
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 3,
            'expected self, notification, and IPC-frame copies')
    require(bundle.count('seL4_CNode_Mint(owned_cnode.cptr,') == 1,
            'expected exactly one badged target fault endpoint')
    require(bundle.index(rollback) < bundle.index(map_page) < bundle.index(configure) <
            bundle.index(write) < bundle.index(read) < bundle.index(resume) <
            bundle.index(receive) < bundle.index(move),
            'rollback/map/configure/write/read/resume/fault/move order')
    require(bundle.count(resume) == 1 and bundle.count(receive) == 1,
            'expected exactly one root resume and one fault receive')
    require('seL4_Fault_VMFault' in bundle and
            'seL4_VMFault_IP) != 0u' in bundle and
            'seL4_VMFault_Addr) != 0u' in bundle and
            'seL4_VMFault_PrefetchFault) == 0u' in bundle and
            'FAULT_BADGE' in bundle,
            'exact VM fault predicate missing')
    bundle_without_comments = re.sub(r'/\*.*?\*/', '', bundle, flags=re.DOTALL)
    require('seL4_Reply' not in bundle_without_comments, 'fault reply must be withheld')
    require('seL4_UserContext zero_context = {0};' in bundle and
            'X86_64_NORMALIZED_RFLAGS' in bundle,
            'zero-request plus normalized-RFLAGS context contract missing')
    for marker in ('seL4_TCB_SetPriority', 'seL4_TCB_SetSchedParams',
                   'seL4_TCB_SetTLSBase', 'seL4_X86_Page_Map',
                   'sel4utils_set_instruction_pointer',
                   'sel4utils_set_stack_pointer', 'clone', 'fork', 'pthread'):
        require(marker not in bundle + server + probe,
                f'forbidden operation: {marker}')
    require('target not resumed' in server and
            'SELINOS_TASKD_FAULT_WITNESS_M0_OWNED' in probe and
            'SELINOS_TASKD_FAULT_WITNESS_M0_REJECTED' in probe,
            'status-only fault-blocked ownership transaction missing')
    for marker in ('seL4_TCB_Resume', 'seL4_Reply', 'not proof that the target completed an instruction',
                   'Linux thread/process'):
        require(marker in gate, f'gate scope marker: {marker}')
    print('SeLinOS fault-mediated first-execution M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
