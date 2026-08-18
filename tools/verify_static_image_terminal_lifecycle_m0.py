#!/usr/bin/env python3
import hashlib
import json
import sys
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, description):
    if not condition:
        raise RuntimeError(description)


def main():
    root = Path(__file__).resolve().parent.parent
    evidence = json.loads((root / 'tests/artifacts/selinos_static_image_terminal_lifecycle_m0.verification.json').read_text())
    require(evidence['profile']['cmake_option'] == 'SeLinStaticImageTerminalLifecycleProbe=ON', 'profile option')
    for group in ('images', 'implementation'):
        for name, binding in evidence[group].items():
            path = root / binding['path']
            require(path.is_file() and sha256(path) == binding['sha256'], f'{group}:{name}')
    runtime = root / evidence['runtime']['path']
    require(runtime.is_file() and sha256(runtime) == evidence['runtime']['sha256'], 'runtime hash')
    transcript = runtime.read_text(errors='replace')
    for marker in evidence['runtime']['required_markers']:
        require(marker in transcript, f'transcript marker: {marker}')
    source = (root / evidence['implementation']['root']['path']).read_text()
    cmake = (root / evidence['implementation']['cmake']['path']).read_text()
    gate = (root / evidence['implementation']['gate']['path']).read_text()
    protocol = (root / evidence['implementation']['protocol']['path']).read_text()
    record = (root / evidence['implementation']['record']['path']).read_text()
    require('SeLinStaticImageTerminalLifecycleProbe' in cmake and 'SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE' in cmake, 'default-off profile')
    for marker in ('TERMINAL_IP', 'INVALID_OPCODE_VECTOR'):
        require(marker in protocol, f'protocol:{marker}')
    for marker in ('terminal_ip', 'terminal_vector', 'tcb_observed', 'cnode_observed', 'vspace_observed', 'entry_frame_observed', 'stack_frame_observed', 'ipc_frame_observed'):
        require(marker in record, f'record:{marker}')
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('#endif', begin)
    body = source[begin:end]
    for marker in ('selinos_static_image_terminal_lifecycle_m0_record', 'SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_TERMINAL_IP', 'SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_INVALID_OPCODE_VECTOR', 'one terminal ownership record observed'):
        require(marker in body, f'root:{marker}')
    observation = body.index('selinos_static_image_terminal_lifecycle_m0_record')
    terminal_validation = body.index('seL4_Fault_UserException')
    require(observation > terminal_validation, 'observation after terminal validation')
    lifecycle = body[observation:]
    for forbidden in ('seL4_Reply(', 'seL4_TCB_Resume(', 'vka_free_object(', 'seL4_CNode_Delete(', 'seL4_X86_Page_Unmap(', 'sel4utils_configure_process('):
        require(forbidden not in lifecycle, f'forbidden post-terminal operation:{forbidden}')
    require('does not prove' in gate and 'cleanup' in gate and 'Linux ABI' in gate, 'explicit boundary')
    print('SeLinOS static-image terminal lifecycle M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
