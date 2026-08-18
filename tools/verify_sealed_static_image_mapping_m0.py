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
    record = json.loads((root / 'tests/artifacts/selinos_sealed_static_image_mapping_m0.verification.json').read_text())
    require(record['profile']['cmake_option'] == 'SeLinSealedStaticImageMappingProbe=ON', 'profile')
    for group in ('images', 'implementation'):
        for name, item in record[group].items():
            path = root / item['path']
            require(path.is_file() and digest(path) == item['sha256'], f'{group}:{name}')
    runtime = root / record['runtime']['path']
    require(runtime.is_file() and digest(runtime) == record['runtime']['sha256'], 'runtime')
    transcript = runtime.read_text(errors='replace')
    for marker in record['runtime']['required_markers']:
        require(marker in transcript, f'marker:{marker}')
    source = (root / record['implementation']['root']['path']).read_text()
    cmake = (root / record['implementation']['cmake']['path']).read_text()
    protocol = (root / record['implementation']['protocol']['path']).read_text()
    gate = (root / record['implementation']['gate']['path']).read_text()
    begin = source.index('static bool start_sealed_static_image_mapping_m0')
    end = source.index('\n}\n#endif\n\nbool selinos_domain_manager_start', begin) + 2
    body = source[begin:end]
    for marker in ('SeLinSealedStaticImageMappingProbe', 'SELINOS_SEALED_STATIC_IMAGE_MAPPING_PROBE'):
        require(marker in cmake, f'cmake:{marker}')
    for marker in ('FIXED_ENTRY_VADDR', 'FIXED_POST_NOP_FAULT_VADDR', 'FIXED_STACK_POINTER', 'PAYLOAD_BYTES', 'FAULT_BADGE'):
        require('SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_' + marker in protocol, f'protocol:{marker}')
    for marker in ('selinos_sealed_static_image_m0_parse', 'selinos_kabi_sha256', 'vspace_map_pages(vspace, &target_entry_frame.cptr', 'vspace_unmap_pages(vspace, root_entry_mapping', 'target_entry_frame.cptr,\n                           (void *)SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR', 'seL4_TCB_Resume(target_tcb.cptr)', 'seL4_Fault_UserException', 'seL4_UserException_FaultIP', 'seL4_UserException_Number'):
        require(marker in body, f'root:{marker}')
    order = [body.index('selinos_sealed_static_image_m0_parse'), body.index('vspace_map_pages(vspace, &target_entry_frame.cptr'), body.index('vspace_unmap_pages(vspace, root_entry_mapping'), body.index('target_entry_frame.cptr,\n                           (void *)SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR'), body.index('seL4_TCB_Resume(target_tcb.cptr)'), body.index('seL4_Fault_UserException')]
    require(order == sorted(order), 'parse/materialize/unmap/map/resume/fault order')
    require(body.count('seL4_TCB_Resume(target_tcb.cptr)') == 1, 'one resume')
    require('seL4_Reply(' not in body, 'no terminal reply')
    for forbidden in ('seL4_X86_Page_Unmap', 'elf_load', 'elf_parse', 'reloc(', 'fork(', 'clone(', 'pthread_'):
        require(forbidden not in body.lower(), f'forbidden:{forbidden}')
    require(('does not prove' in gate or 'will not prove' in gate) and
            'ELF' in gate and 'Linux ABI' in gate, 'gate boundary')
    print('SeLinOS sealed static-image mapping M0 evidence verified.')


try:
    main()
except (RuntimeError, KeyError, ValueError) as error:
    print('verification failed: ' + str(error), file=sys.stderr)
    sys.exit(1)
