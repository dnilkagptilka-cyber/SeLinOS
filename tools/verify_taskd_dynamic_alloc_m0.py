#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path
def d(p):
 h=hashlib.sha256();h.update(p.read_bytes());return h.hexdigest()
def q(x,m):
 if not x:raise RuntimeError(m)
def main():
 r=Path(__file__).resolve().parent.parent;e=json.loads((r/'tests/artifacts/selinos_taskd_dynamic_alloc_m0.verification.json').read_text())
 q(e['schema']==1,'schema');q(e['profile']['cmake_option']=='SeLinTaskdDynamicAllocationProbe=ON','gate')
 for group in ('images','implementation'):
  for n,i in e[group].items():
   p=r/i['path'];q(p.is_file() and d(p)==i['sha256'],f'{n} sha')
 log=(r/e['runtime']['path']).read_text();q(d(r/e['runtime']['path'])==e['runtime']['sha256'],'log sha')
 for x in e['runtime']['required_markers']:q(x in log,f'marker {x}')
 s=(r/e['implementation']['server']['path']).read_text();p=(r/e['implementation']['probe']['path']).read_text();root=(r/e['implementation']['root']['path']).read_text();cm=(r/e['implementation']['cmake']['path']).read_text();proto=(r/e['implementation']['protocol']['path']).read_text()
 for x in ('SeLinTaskdDynamicAllocationProbe','SELINOS_TASKD_DYNAMIC_ALLOCATION_PROBE','selinos-taskd-dynamic-alloc-m0'):q(x in cm,f'cmake {x}')
 for x in ('SELINOS_TASKD_DYN_M0_RESERVE','SELINOS_TASKD_DYN_M0_RELEASE','SELINOS_TASKD_DYN_M0_EBUSY'):q(x in proto,f'protocol {x}')
 for x in ('bool reserved = false;','reserved = true; generation++;','reserved = false;','SELINOS_TASKD_DYN_M0_EBUSY'):q(x in s,f'server {x}')
 for x in ('SELINOS_TASKD_DYN_M0_RESERVED,1u','SELINOS_TASKD_DYN_M0_EBUSY,1u','SELINOS_TASKD_DYN_M0_RELEASED,1u','SELINOS_TASKD_DYN_M0_RESERVED,2u'):q(x in p,f'probe {x}')
 q('start_taskd_dynamic_alloc_m0_bundle' in root and 'no TCB/CSpace/VSpace or Linux process created.' in root,'root bound')
 for x in ('seL4_TCB_Configure','seL4_TCB_Resume','sel4utils_spawn_process_v(&child','clone','fork','pthread'):q(x not in s+p,'forbidden '+x)
 print('SeLinOS taskd dynamic allocation prerequisite M0 evidence verified.')
try:main()
except (RuntimeError,KeyError,ValueError) as x:print('verification failed:',x,file=sys.stderr);sys.exit(1)
