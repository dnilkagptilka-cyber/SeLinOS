#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, description):
    if not condition:
        raise RuntimeError(description)


def main():
    root = Path(__file__).resolve().parent.parent
    evidence = json.loads((root / 'tests/artifacts/selinos_static_image_target_capability_deletion_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinStaticImageTargetCapabilityDeletionProbe=ON', 'profile')
    for group in ('images', 'implementation'):
        for name, binding in evidence[group].items():
            path = root / binding['path']
            require(path.is_file() and digest(path) == binding['sha256'], f'{group}:{name}')
    runtime = root / evidence['runtime']['path']
    require(runtime.is_file() and digest(runtime) == evidence['runtime']['sha256'], 'runtime hash')
    transcript = runtime.read_text(errors='replace')
    for marker in evidence['runtime']['required_markers']:
        require(marker in transcript, f'transcript:{marker}')
    source = (root / evidence['implementation']['root']['path']).read_text()
    cmake = (root / evidence['implementation']['cmake']['path']).read_text()
    gate = (root / evidence['implementation']['gate']['path']).read_text()
    protocol = (root / evidence['implementation']['protocol']['path']).read_text()
    require('SeLinStaticImageTargetCapabilityDeletionProbe' in cmake and 'SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE' in cmake, 'default-off profile')
    for marker in ('SELF_SLOT', 'NOTIFICATION_SLOT', 'IPC_FRAME_SLOT', 'FAULT_ENDPOINT_SLOT', 'ENTRY_FRAME_SLOT', 'STACK_FRAME_SLOT', 'TERMINAL_IP', 'INVALID_OPCODE_VECTOR'):
        require(marker in protocol, f'protocol:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    terminal = body.index('seL4_Fault_UserException')
    unmap_entry = body.index('seL4_X86_Page_Unmap(target_entry_frame.cptr)')
    unmap_stack = body.index('seL4_X86_Page_Unmap(target_stack_frame.cptr)')
    unmap_ipc = body.index('seL4_X86_Page_Unmap(target_ipc_frame.cptr)')
    require(terminal < unmap_entry < unmap_stack < unmap_ipc, 'terminal-entry-stack-ipc prerequisite order')
    phase70 = body.index('#if CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE')
    phase70_end = body.index('\n#endif', phase70) + len('\n#endif')
    transaction = body[phase70:phase70_end]
    slots = ('NOTIFICATION_SLOT', 'IPC_FRAME_SLOT', 'FAULT_ENDPOINT_SLOT', 'ENTRY_FRAME_SLOT', 'STACK_FRAME_SLOT', 'SELF_SLOT')
    positions = []
    for slot in slots:
        marker = 'SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_' + slot
        require(marker in transaction, f'target-slot:{slot}')
        positions.append(transaction.index(marker))
    require(positions == sorted(positions), 'notification-ipc-fault-entry-stack-self deletion order')
    require(transaction.count('seL4_CNode_Delete(target_cnode.cptr,') == 6, 'exactly six target-CNode deletions')
    require('seL4_CNode_Delete(seL4_CapInitThreadCNode' not in transaction, 'no root-CNode deletion')
    for forbidden in ('seL4_Reply(', 'seL4_TCB_Resume(', 'vka_free_object(', 'seL4_X86_ASIDPool_Assign(', 'vka_alloc_', 'sel4utils_map_page', 'seL4_X86_Page_Map(', 'sel4utils_configure_process('):
        require(forbidden not in transaction, f'forbidden post-terminal operation:{forbidden}')
    require('does not prove' in gate and 'root capability deletion' in gate and 'object destruction' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS static-image target capability deletion M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
