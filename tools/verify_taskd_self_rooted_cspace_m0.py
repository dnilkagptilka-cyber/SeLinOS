#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path

def d(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def r(c,m):
    if not c: raise RuntimeError(m)
def main():
    root=Path(__file__).resolve().parent.parent
    x=json.loads((root/'tests/artifacts/selinos_taskd_self_rooted_cspace_m0.verification.json').read_text())
    r(x['profile']['cmake_option']=='SeLinTaskdSelfRootedCspaceProbe=ON','gate')
    for g in ('images','implementation'):
        for n,v in x[g].items(): r((root/v['path']).is_file() and d(root/v['path'])==v['sha256'],g+':'+n)
    log=root/x['runtime']['path']; r(d(log)==x['runtime']['sha256'],'runtime hash')
    t=log.read_text(errors='replace')
    for m in x['runtime']['required_markers']: r(m in t,'marker '+m)
    src=(root/x['implementation']['root']['path']).read_text(); cm=(root/x['implementation']['cmake']['path']).read_text(); p=(root/x['implementation']['protocol']['path']).read_text(); s=(root/x['implementation']['server']['path']).read_text(); q=(root/x['implementation']['probe']['path']).read_text(); gate=(root/x['implementation']['gate']['path']).read_text()
    for m in ('SeLinTaskdSelfRootedCspaceProbe','SELINOS_TASKD_SELF_ROOTED_CSPACE_PROBE','selinos-taskd-self-rooted-cspace-m0'): r(m in cm,'cmake '+m)
    for m in ('TARGET_CNODE_SELF_SLOT','TARGET_NOTIFICATION_SLOT','CNODE_SLOT_BITS'): r('SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_'+m in p,'protocol '+m)
    a=src.index('static bool start_taskd_self_rooted_cspace_m0'); b=src.index('#endif',a); z=src[a:b]
    cp='seL4_CNode_Copy(owned_cnode.cptr,'; asid='seL4_X86_ASIDPool_Assign('; cfg='seL4_TCB_Configure('; move='sel4utils_move_cap_to_process(&taskd,'
    r(z.count(cp)==2,'two target CNode copies'); r('TARGET_CNODE_SELF_SLOT' in z and 'TARGET_NOTIFICATION_SLOT' in z,'slots 0 and 1'); r(z.index(cp)<z.index(asid)<z.index(cfg)<z.index(move),'copy ASID Configure move order'); r(z.count(move)==3,'three moves'); r(z.index('vka_free_object(vka, &rollback_tcb);')<z.index(cp),'rollback before copies')
    for f in ('seL4_CNode_Mint','seL4_CNode_Move','seL4_CNode_Delete','seL4_TCB_WriteRegisters','seL4_TCB_Resume','seL4_X86_Page_Map','clone','fork','pthread'): r(f not in z+s+q,'forbidden '+f)
    r('target resources not invoked' in s and 'SELF_ROOTED_CSPACE_M0_OWNED' in q and 'SELF_ROOTED_CSPACE_M0_REJECTED' in q,'status transaction')
    r('self-CNode capability in slot `0`' in gate and 'target is never resumed' in gate,'gate boundary')
    print('SeLinOS self-rooted two-cap CSpace configured non-executing task M0 evidence verified.')
try: main()
except (RuntimeError,KeyError,ValueError) as e: print('verification failed: '+str(e),file=sys.stderr);sys.exit(1)
