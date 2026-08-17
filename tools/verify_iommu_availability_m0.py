#!/usr/bin/env python3
import hashlib,json,sys
from pathlib import Path

def h(p):
 d=hashlib.sha256();
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):d.update(b)
 return d.hexdigest()
def req(x,m):
 if not x:raise RuntimeError(m)
def bind(r,i,n):
 p=r/i['path'];req(p.is_file(),f'missing {n}');req(h(p)==i['sha256'],f'{n} SHA mismatch');return p
def main():
 r=Path(__file__).resolve().parent.parent;e=json.loads((r/'tests/artifacts/selinos_iommu_availability_m0.verification.json').read_text());req(e['schema']==1,'schema')
 p=e['profile'];req(p['build_directory']=='build-iommu-availability-probe','profile build');req(p['cmake_option']=='SeLinRootIommuAvailabilityProbe=ON','profile gate');req(p['kernel_iommu_config']=='KernelIOMMU=ON','kernel config');req(p['observed_numIOPTLevels']==0,'expected zero level');req('containment blocker' in p['classification'],'blocker classification')
 for n,i in e['images'].items():req(bind(r,i,n).stat().st_size>0,f'empty {n}')
 s={n:bind(r,i,n).read_text(errors='replace') for n,i in e['implementation'].items()};log=bind(r,e['runtime_evidence'],'log').read_text(errors='replace')
 for x in ('SeLinRootIommuAvailabilityProbe','SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE','Enable root-only x86 IOMMU bootinfo availability observation'):req(x in s['build_gate'],f'gate {x}')
 for x in ('CONFIG_SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE','sel4runtime_bootinfo()','iommu_bootinfo->numIOPTLevels == 0u','numIOPTLevels=0; QEMU containment blocker observed.'):req(x in s['root_wiring'],f'wiring {x}')
 for x in ('seL4_X86_IOPageTable_Map','seL4_X86_IOSpace','vka_alloc_dma','PCI_COMMAND_MASTER','selinos_pci_') : req(x not in s['root_wiring'][s['root_wiring'].index('#if CONFIG_SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE'):s['root_wiring'].index('#endif',s['root_wiring'].index('#if CONFIG_SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE'))],f'forbidden {x}')
 for x in e['runtime_evidence']['required_markers']:req(x in log,f'missing marker {x}')
 for x in e['runtime_evidence']['forbidden_markers']:req(x not in log,f'forbidden marker {x}')
 for x in ('Status: verified, bounded M0 blocker proof.','numIOPTLevels','does not configure an IOMMU','Acceptance and non-claims','dpkg` or `apt`'):req(x in s['phase_design_gate'],f'design {x}')
 nc=' '.join(e['not_claimed']);
 for x in ('IOMMU configuration','DMA behavior','storage I/O','dpkg or apt'):req(x in nc,f'nonclaim {x}')
 print('SeLinOS root-only x86 IOMMU bootinfo availability M0 evidence verified.')
if __name__=='__main__':
 try:main()
 except (RuntimeError,KeyError,ValueError) as x:print(f'verification failed: {x}',file=sys.stderr);sys.exit(1)
