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
    record = json.loads((root / 'tests/artifacts/selinos_taskd_vm_restart_m0.verification.json').read_text())
    require(record['profile']['cmake_option'] == 'SeLinTaskdVmRestartProbe=ON', 'profile')
    for group in ('images', 'implementation'):
        for name, item in record[group].items():
            path = root / item['path']
            require(path.is_file() and digest(path) == item['sha256'], f'{group}:{name} hash')
    runtime = root / record['runtime']['path']
    require(digest(runtime) == record['runtime']['sha256'], 'runtime hash')
    text = runtime.read_text(errors='replace')
    for marker in record['runtime']['required_markers']:
        require(marker in text, f'runtime marker: {marker}')

    source = (root / record['implementation']['root']['path']).read_text()
    cmake = (root / record['implementation']['cmake']['path']).read_text()
    protocol = (root / record['implementation']['protocol']['path']).read_text()
    server = (root / record['implementation']['server']['path']).read_text()
    probe = (root / record['implementation']['probe']['path']).read_text()
    gate = (root / record['implementation']['gate']['path']).read_text()
    for marker in ('SeLinTaskdVmRestartProbe', 'SELINOS_TASKD_VM_RESTART_PROBE', 'selinos-taskd-vm-restart-m0'):
        require(marker in cmake, f'CMake: {marker}')
    for marker in ('FIXED_ENTRY_VADDR', 'FIXED_POST_NOP_FAULT_VADDR', 'FIXED_STACK_POINTER',
                   'X86_INVALID_OPCODE_VECTOR', 'NOP_OPCODE', 'UD2_OPCODE_0', 'UD2_OPCODE_1'):
        require('SELINOS_TASKD_VM_RESTART_M0_' + marker in protocol, f'protocol: {marker}')
    begin = source.index('static bool start_taskd_vm_restart_m0')
    end = source.index('#endif', begin)
    bundle = source[begin:end]
    require(bundle.count('seL4_CNode_Copy(owned_cnode.cptr,') == 5, 'five target cap copies')
    require(bundle.count('seL4_CNode_Mint(owned_cnode.cptr,') == 1, 'one fault endpoint mint')
    require(bundle.count('seL4_TCB_Resume(owned_tcb.cptr)') == 1, 'one explicit resume')
    uncommented = re.sub(r'/\*.*?\*/', '', bundle, flags=re.DOTALL)
    require(uncommented.count('seL4_Reply(') == 1, 'one supported VM-fault reply')
    required = ('vspace_map_pages(vspace, &target_entry_frame.cptr',
                'vspace_unmap_pages(vspace, root_entry_mapping',
                'seL4_Fault_VMFault', 'seL4_VMFault_IP', 'seL4_VMFault_Addr',
                'seL4_VMFault_PrefetchFault', 'sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,\n                           target_entry_frame.cptr',
                'seL4_Fault_UserException', 'seL4_UserException_FaultIP',
                'seL4_UserException_Number', 'X86_INVALID_OPCODE_VECTOR')
    for marker in required:
        require(marker in bundle, f'root: {marker}')
    order = [bundle.index('seL4_TCB_Resume(owned_tcb.cptr)'),
             bundle.index('seL4_Fault_VMFault'),
             bundle.index('target_entry_frame.cptr,\n                           (void *)SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR'),
             bundle.index('seL4_Reply('),
             bundle.index('seL4_Fault_UserException'),
             bundle.index('sel4utils_move_cap_to_process(&taskd,')]
    require(order == sorted(order), 'resume/VMfault/map/reply/UserException/delegate order')
    for forbidden in ('seL4_TCB_Suspend', 'seL4_UserException_FaultIP);\n    seL4_Reply',
                      'clone', 'fork', 'pthread', 'seL4_X86_Page_Unmap'):
        require(forbidden not in bundle + server + probe, f'forbidden: {forbidden}')
    require('UserException reply' in gate and 'one zero-label' in gate and
            'Linux ABI' in gate and 'seL4_Fault_VMFault' in gate,
            'gate scope')
    print('SeLinOS VM-fault reply restart M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
