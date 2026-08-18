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
        (root / 'tests/artifacts/selinos_taskd_entry_stack_m0.verification.json').read_text()
    )
    require(record['profile']['cmake_option'] == 'SeLinTaskdEntryStackProbe=ON',
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

    for marker in ('SeLinTaskdEntryStackProbe',
                   'SELINOS_TASKD_ENTRY_STACK_PROBE',
                   'selinos-taskd-entry-stack-m0'):
        require(marker in cmake, f'CMake marker: {marker}')
    for marker in ('TARGET_CNODE_SELF_SLOT', 'TARGET_NOTIFICATION_SLOT',
                   'TARGET_IPC_FRAME_SLOT', 'TARGET_FAULT_ENDPOINT_SLOT',
                   'TARGET_ENTRY_FRAME_SLOT', 'TARGET_STACK_FRAME_SLOT',
                   'FIXED_ENTRY_VADDR', 'FIXED_STACK_VADDR',
                   'FIXED_STACK_POINTER', 'X86_64_NORMALIZED_RFLAGS',
                   'X86_64_STACK_ENTRY_MODULO'):
        require('SELINOS_TASKD_ENTRY_STACK_M0_' + marker in protocol,
                f'protocol marker: {marker}')

    begin = source.index('static bool start_taskd_entry_stack_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 5,
            'expected self, notification, IPC, entry, and stack frame copies')
    require(bundle.count('seL4_CNode_Mint(owned_cnode.cptr,') == 1,
            'expected exactly one target fault endpoint mint')
    require(bundle.count('sel4utils_map_page_with_attributes(') == 2,
            'expected exactly two NX provenance leaf mappings')
    require('seL4_X86_ExecuteDisable' in bundle and
            'target_entry_frame.cptr' in bundle and
            'target_stack_frame.cptr' in bundle,
            'NX entry/stack mapping contract missing')
    require('entry_stack_m0_is_low_canonical' in bundle and
            'entry_stack_m0_is_abi_entry_stack' in bundle and
            '0xffff800000000000ull' in bundle,
            'canonical/alignment negative guard missing')
    require('requested_context.rip = SELINOS_TASKD_ENTRY_STACK_M0_FIXED_ENTRY_VADDR;' in bundle and
            'requested_context.rsp = SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER;' in bundle and
            'X86_64_NORMALIZED_RFLAGS' in bundle,
            'entry/stack context write/read-back contract missing')
    configure = 'seL4_TCB_Configure(owned_tcb.cptr,'
    write = 'seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,'
    read = 'seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,'
    move = 'sel4utils_move_cap_to_process(&taskd,'
    require(bundle.index(configure) < bundle.index(write) < bundle.index(read) <
            bundle.index(move), 'configure/write/read/move order')
    for forbidden in ('seL4_TCB_Resume', 'seL4_Reply', 'seL4_Recv(fault_endpoint',
                      'seL4_X86_Page_Map', 'write_fixed_ret_opcode',
                      'sel4utils_set_instruction_pointer',
                      'sel4utils_set_stack_pointer', 'clone', 'fork', 'pthread'):
        require(forbidden not in bundle, f'forbidden root operation: {forbidden}')
    for forbidden in ('seL4_TCB_Resume', 'seL4_Recv(fault_endpoint',
                      'seL4_X86_Page_Map', 'write_fixed_ret_opcode',
                      'sel4utils_set_instruction_pointer',
                      'sel4utils_set_stack_pointer', 'clone', 'fork', 'pthread'):
        require(forbidden not in server + probe,
                f'forbidden witness operation: {forbidden}')
    require('target not resumed' in server and
            'SELINOS_TASKD_ENTRY_STACK_M0_OWNED' in probe and
            'SELINOS_TASKD_ENTRY_STACK_M0_REJECTED' in probe,
            'status-only ownership transaction missing')
    for marker in ('seL4_X86_ExecuteDisable', 'seL4_TCB_Resume',
                   'successful instruction execution', 'Linux thread/process'):
        require(marker in gate, f'gate scope marker: {marker}')
    print('SeLinOS entry-point and stack provenance M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
