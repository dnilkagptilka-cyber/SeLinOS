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
    evidence = json.loads((root / 'tests/artifacts/selinos_fresh_target_bundle_construction_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinFreshTargetBundleConstructionProbe=ON', 'profile')
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
    require('SeLinFreshTargetBundleConstructionProbe' in cmake and 'SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE' in cmake, 'default-off profile')
    for marker in ('TERMINAL_IP', 'INVALID_OPCODE_VECTOR', 'GENERATION', 'CNODE_SLOT_BITS', 'OBJECT_COUNT'):
        require(marker in protocol, f'protocol:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    authorization = body.index('generation 2 authorized once; duplicate and retired generation 1 rejected')
    fresh_tcb_allocation = body.index('vka_alloc_tcb(vka, &fresh_tcb)')
    phase73 = body.rfind('#if ', 0, fresh_tcb_allocation)
    phase73_end = body.index('\n#endif', phase73) + len('\n#endif')
    transaction = body[phase73:phase73_end]
    require(authorization < phase73, 'Phase 72 authorization precedes construction')
    allocations = ('vka_alloc_tcb(', 'vka_alloc_cnode_object(', 'vka_alloc_vspace_root(', 'vka_alloc_notification(', 'vka_alloc_frame(vka, seL4_PageBits, &fresh_ipc_frame)', 'vka_alloc_frame(vka, seL4_PageBits, &fresh_entry_frame)', 'vka_alloc_frame(vka, seL4_PageBits, &fresh_stack_frame)')
    positions = []
    for marker in allocations:
        require(marker in transaction, f'fresh allocation:{marker}')
        positions.append(transaction.index(marker))
    require(positions == sorted(positions), 'TCB-CNode-PML4-notification-IPC-entry-stack allocation order')
    require(transaction.count('vka_alloc_') == 7, 'exact seven fresh allocations')
    for forbidden in ('vka_free_object(', 'seL4_CNode_Delete(', 'seL4_CNode_Copy(', 'seL4_CNode_Mint(', 'seL4_X86_ASIDPool_Assign(', 'sel4utils_map_page', 'seL4_X86_Page_Map(', 'seL4_TCB_Configure(', 'seL4_TCB_Resume(', 'seL4_Reply(', 'seL4_TCB_WriteRegisters(', 'memcpy('):
        require(forbidden not in transaction, f'forbidden fresh-construction operation:{forbidden}')
    require('does not prove' in gate and 'physical resource reuse' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS fresh target-bundle construction M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
