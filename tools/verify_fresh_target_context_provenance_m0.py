#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path

def h(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def req(v,m):
 if not v: raise RuntimeError(m)
try:
 root=Path(__file__).resolve().parent.parent;e=json.loads((root/'tests/artifacts/selinos_fresh_target_context_provenance_m0.verification.json').read_text())
 for g in ('images','implementation'):
  for _,b in e[g].items(): req(h(root/b['path'])==b['sha256'],g)
 r=root/e['runtime']['path'];req(h(r)==e['runtime']['sha256'],'runtime')
 for x in e['runtime']['required_markers']: req(x in r.read_text(errors='replace'),'marker')
 s=(root/e['implementation']['root']['path']).read_text();c=(root/e['implementation']['cmake']['path']).read_text();req('SeLinFreshTargetContextProvenanceProbe' in c,'profile')
 a=s.index('requested_context.rip = SELINOS_FRESH_TARGET_CONTEXT_PROVENANCE_M0_FIXED_RIP');b=s.rfind('#if ',0,a);z=s.index('\n#endif',b);q=s[b:z]
 for x in ('seL4_TCB_WriteRegisters(fresh_tcb.cptr','seL4_TCB_ReadRegisters(fresh_tcb.cptr','observed_context.rip != requested_context.rip','observed_context.rsp != requested_context.rsp'): req(x in q,x)
 for x in ('seL4_TCB_Resume(','seL4_Reply(','vka_alloc_','vka_free_object(','seL4_X86_Page_Map('): req(x not in q,x)
 print('SeLinOS fresh target context provenance M0 evidence verified.')
except Exception as x: print('verification failed: '+str(x),file=sys.stderr);sys.exit(1)
