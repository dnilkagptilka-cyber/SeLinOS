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
    evidence = json.loads((root / 'tests/artifacts/selinos_static_image_mapping_revocation_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinStaticImageMappingRevocationProbe=ON', 'profile')
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
    require('SeLinStaticImageMappingRevocationProbe' in cmake and 'SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE' in cmake, 'default-off profile')
    for marker in ('ENTRY_VADDR', 'STACK_VADDR', 'IPC_VADDR', 'TERMINAL_IP', 'INVALID_OPCODE_VECTOR'):
        require(marker in protocol, f'protocol:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    terminal = body.index('seL4_Fault_UserException')
    revocation = body.index('seL4_X86_Page_Unmap(target_entry_frame.cptr)')
    require(revocation > terminal, 'unmaps after terminal validation')
    post = body[revocation:]
    entry = post.index('seL4_X86_Page_Unmap(target_entry_frame.cptr)')
    stack = post.index('seL4_X86_Page_Unmap(target_stack_frame.cptr)')
    ipc = post.index('seL4_X86_Page_Unmap(target_ipc_frame.cptr)')
    require(entry < stack < ipc, 'entry-stack-ipc unmap order')
    require(post.count('seL4_X86_Page_Unmap(') == 3, 'exactly three target unmaps')
    for forbidden in ('seL4_Reply(', 'seL4_TCB_Resume(', 'vka_free_object(', 'seL4_CNode_Delete(', 'seL4_X86_ASIDPool_Assign(', 'sel4utils_configure_process('):
        require(forbidden not in post, f'forbidden post-terminal operation:{forbidden}')
    require('does not prove' in gate and 'frame reclamation' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS static-image mapping revocation M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
