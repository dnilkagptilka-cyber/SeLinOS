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
    evidence = json.loads((root / 'tests/artifacts/selinos_static_image_fresh_bundle_authorization_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinStaticImageFreshBundleAuthorizationProbe=ON', 'profile')
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
    require('SeLinStaticImageFreshBundleAuthorizationProbe' in cmake and 'SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE' in cmake, 'default-off profile')
    for marker in ('TERMINAL_IP', 'INVALID_OPCODE_VECTOR', 'RETIRED_GENERATION', 'FRESH_GENERATION', 'APPROVED', 'REJECTED_STALE'):
        require(marker in protocol, f'protocol:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    terminal = body.index('seL4_Fault_UserException')
    unmap = body.index('seL4_X86_Page_Unmap(target_entry_frame.cptr)')
    delete = body.index('SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_NOTIFICATION_SLOT')
    reclaim = body.index('vka_free_object(vka, &target_tcb);')
    phase72 = body.index('#if CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE')
    phase72_end = body.index('\n#endif', phase72) + len('\n#endif')
    transaction = body[phase72:phase72_end]
    require(terminal < unmap < delete < reclaim < phase72, 'terminal-cleanup-authorization phase order')
    for marker in ('retired_generation', 'fresh_generation', 'authorization_used', 'FRESH_BUNDLE_AUTHORIZATION_M0_APPROVED', 'FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE', 'generation 2 authorized once; duplicate and retired generation 1 rejected'):
        require(marker in transaction, f'authorization:{marker}')
    require(transaction.count('FRESH_BUNDLE_AUTHORIZATION_M0_APPROVED') == 3, 'single approval and two comparison uses')
    require(transaction.count('FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE') == 4, 'two stale assignments and two comparisons')
    for forbidden in ('vka_alloc_', 'vka_free_object(', 'seL4_CNode_Delete(', 'seL4_CNode_Copy(', 'seL4_CNode_Mint(', 'seL4_Reply(', 'seL4_TCB_Resume(', 'seL4_TCB_Configure(', 'sel4utils_map_page', 'seL4_X86_Page_Map(', 'seL4_X86_ASIDPool_Assign(', 'sel4utils_configure_process('):
        require(forbidden not in transaction, f'forbidden authorization operation:{forbidden}')
    require('does not prove' in gate and 'actual fresh allocation' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS static-image fresh-bundle authorization M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
