#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path

def h(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def req(v,m):
    if not v: raise RuntimeError(m)
try:
 root=Path(__file__).resolve().parent.parent
 e=json.loads((root/'tests/artifacts/selinos_fresh_target_bundle_configuration_m0.verification.json').read_text())
 for g in ('images','implementation'):
  for _,b in e[g].items(): req(h(root/b['path'])==b['sha256'],g)
 r=root/e['runtime']['path']; req(h(r)==e['runtime']['sha256'],'runtime')
 t=r.read_text(errors='replace')
 for x in e['runtime']['required_markers']: req(x in t,'marker')
 s=(root/e['implementation']['root']['path']).read_text(); c=(root/e['implementation']['cmake']['path']).read_text()
 req('SeLinFreshTargetBundleConfigurationProbe' in c,'profile')
 a=s.index('vka_alloc_endpoint(vka, &fresh_fault_endpoint)'); b=s.rfind('#if ',0,a); z=s.index('\n#endif',b); q=s[b:z]
 for x in ('seL4_CNode_Copy(fresh_cnode.cptr,','seL4_CNode_Mint(fresh_cnode.cptr,','seL4_X86_ASIDPool_Assign(','sel4utils_map_page(','seL4_TCB_Configure(fresh_tcb.cptr') : req(x in q,x)
 for x in ('seL4_TCB_WriteRegisters(','seL4_TCB_Resume(','seL4_Reply(','vka_free_object(','vka_alloc_tcb(','seL4_X86_Page_Map('): req(x not in q,x)
 print('SeLinOS fresh target-bundle configuration M0 evidence verified.')
except Exception as x:
 print('verification failed: '+str(x),file=sys.stderr);sys.exit(1)
