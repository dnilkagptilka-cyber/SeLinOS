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
    evidence = json.loads((root / 'tests/artifacts/selinos_static_image_object_reclamation_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinStaticImageObjectReclamationProbe=ON', 'profile')
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
    require('SeLinStaticImageObjectReclamationProbe' in cmake and 'SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE' in cmake, 'default-off profile')
    for marker in ('TERMINAL_IP', 'INVALID_OPCODE_VECTOR', 'MAX_PAGING_OBJECTS', 'TCB_ORDER', 'FRAME_ORDER', 'PAGING_ORDER', 'CNODE_ORDER', 'PML4_ORDER', 'ENDPOINT_NOTIFICATION_ORDER'):
        require(marker in protocol, f'protocol:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    terminal = body.index('seL4_Fault_UserException')
    unmap_entry = body.index('seL4_X86_Page_Unmap(target_entry_frame.cptr)')
    delete_notification = body.index('SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_NOTIFICATION_SLOT')
    phase71 = body.index('#if CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE')
    phase71_end = body.index('\n#endif', phase71) + len('\n#endif')
    transaction = body[phase71:phase71_end]
    require(terminal < unmap_entry < delete_notification < phase71, 'terminal-unmap-delete-reclamation phase order')
    frees = (
        'vka_free_object(vka, &target_tcb);',
        'vka_free_object(vka, &target_entry_frame);',
        'vka_free_object(vka, &target_stack_frame);',
        'vka_free_object(vka, &target_ipc_frame);',
        'for (register_index = (seL4_Word)paging_object_count;',
        'vka_free_object(vka, &target_cnode);',
        'vka_free_object(vka, &target_vspace_root);',
        'vka_free_object(vka, &fault_endpoint);',
        'vka_free_object(vka, &target_notification);',
    )
    positions = []
    for marker in frees:
        require(marker in transaction, f'reclamation:{marker}')
        positions.append(transaction.index(marker))
    require(positions == sorted(positions), 'TCB-frames-reverse-paging-CNode-PML4-endpoint-notification order')
    require('vka_free_object(vka, &paging_objects[register_index - 1u]);' in transaction, 'reverse paging-object disposal')
    require(transaction.count('vka_free_object(vka,') == 9, 'fixed root-object disposal call count')
    for forbidden in ('seL4_Reply(', 'seL4_TCB_Resume(', 'vka_alloc_', 'seL4_TCB_Configure(', 'sel4utils_map_page', 'seL4_X86_Page_Map(', 'seL4_X86_ASIDPool_Assign(', 'seL4_CNode_Copy(', 'seL4_CNode_Mint(', 'sel4utils_configure_process('):
        require(forbidden not in transaction, f'forbidden reclamation operation:{forbidden}')
    require('does not prove' in gate and 'ASID reuse' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS static-image object reclamation M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
