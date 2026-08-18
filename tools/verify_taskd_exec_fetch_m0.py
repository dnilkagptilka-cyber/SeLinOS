#!/usr/bin/env python3
import hashlib
import json
import re
import sys
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    root = Path(__file__).resolve().parent.parent
    record = json.loads((root / 'tests/artifacts/selinos_taskd_exec_fetch_m0.verification.json').read_text())
    require(record['profile']['cmake_option'] == 'SeLinTaskdExecFetchProbe=ON', 'incorrect profile')
    for group in ('images', 'implementation'):
        for name, item in record[group].items():
            path = root / item['path']
            require(path.is_file() and sha256(path) == item['sha256'], f'{group}:{name} hash')
    runtime = root / record['runtime']['path']
    require(sha256(runtime) == record['runtime']['sha256'], 'runtime hash')
    transcript = runtime.read_text(errors='replace')
    for marker in record['runtime']['required_markers']:
        require(marker in transcript, f'missing runtime marker: {marker}')

    source = (root / record['implementation']['root']['path']).read_text()
    cmake = (root / record['implementation']['cmake']['path']).read_text()
    protocol = (root / record['implementation']['protocol']['path']).read_text()
    server = (root / record['implementation']['server']['path']).read_text()
    probe = (root / record['implementation']['probe']['path']).read_text()
    gate = (root / record['implementation']['gate']['path']).read_text()
    for marker in ('SeLinTaskdExecFetchProbe', 'SELINOS_TASKD_EXEC_FETCH_PROBE', 'selinos-taskd-exec-fetch-m0'):
        require(marker in cmake, f'CMake marker: {marker}')
    for marker in ('NOP_OPCODE', 'UD2_OPCODE_0', 'UD2_OPCODE_1', 'WITNESS_BYTES',
                   'FIXED_ENTRY_VADDR', 'FIXED_POST_NOP_FAULT_VADDR',
                   'FIXED_STACK_POINTER', 'X86_INVALID_OPCODE_VECTOR'):
        require('SELINOS_TASKD_EXEC_FETCH_M0_' + marker in protocol, f'protocol: {marker}')
    begin = source.index('static bool start_taskd_exec_fetch_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 5, 'five target frame/cnode copies required')
    require(bundle.count('seL4_CNode_Mint(owned_cnode.cptr,') == 1, 'one badged fault endpoint required')
    for marker in ('vspace_map_pages(vspace, &target_entry_frame.cptr',
                   'NOP_OPCODE', 'UD2_OPCODE_0', 'UD2_OPCODE_1',
                   'vspace_unmap_pages(vspace, root_entry_mapping',
                   'seL4_X86_Default_VMAttributes,', 'seL4_X86_ExecuteDisable',
                   'seL4_TCB_Resume(owned_tcb.cptr)', 'seL4_Fault_UserException',
                   'seL4_UserException_FaultIP', 'seL4_UserException_SP',
                   'seL4_UserException_Number', 'X86_INVALID_OPCODE_VECTOR'):
        require(marker in bundle, f'root marker: {marker}')
    root_without_comments = re.sub(r'/\*.*?\*/', '', bundle, flags=re.DOTALL)
    require(root_without_comments.count('seL4_TCB_Resume(owned_tcb.cptr)') == 1, 'exactly one resume')
    require('seL4_Reply' not in root_without_comments, 'fault reply must be withheld')
    order = [bundle.index('vspace_map_pages(vspace, &target_entry_frame.cptr'),
             bundle.index('vspace_unmap_pages(vspace, root_entry_mapping'),
             bundle.index('sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,\n                           target_entry_frame.cptr'),
             bundle.index('seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,'),
             bundle.index('seL4_TCB_Resume(owned_tcb.cptr)'),
             bundle.index('fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);'),
             bundle.index('sel4utils_move_cap_to_process(&taskd,')]
    require(order == sorted(order), 'initialize/unmap/map/write/resume/fault/delegate order')
    for forbidden in ('clone', 'fork', 'pthread', 'seL4_X86_Page_Unmap', 'seL4_TCB_Suspend'):
        require(forbidden not in bundle + server + probe, f'forbidden operation: {forbidden}')
    require('target not resumed' in server and 'OWNED' in probe and 'REJECTED' in probe,
            'status-only witness missing')
    for marker in ('`nop; ud2`', 'seL4_TCB_Resume', 'not proof of a general process launch', 'Linux ABI'):
        require(marker in gate, f'gate boundary marker: {marker}')
    print('SeLinOS executable instruction-fetch witness M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
