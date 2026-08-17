# SeLinOS KABI M3: controlled native module-execution design

## Purpose

KABI M2 deliberately stops after integrity verification, `__versions` CRC validation and relocation of one real Linux 6.18.44 `ET_REL` fixture. KABI M3 defines the conditions required before SeLinOS may invoke a relocated module entrypoint. It is a design contract, not evidence that native `.ko` execution already works.

> **Invariant:** no module code may execute in `rootd`, `deviced`, `irqd`, `dmad` or `selinos-modld`. A future load receives a fresh TCB, CSpace, VSpace and fault endpoint.

| Stage | Required action | Required negative evidence |
|---|---|---|
| Admission | Verify package signature/trust policy, exact version profile, ELF, vermagic, all import CRCs and a capability manifest | Modified digest/signature, unknown import, bad CRC and wrong vermagic are rejected before memory allocation |
| Layout | Allocate fresh module-domain memory, copy `SHF_ALLOC` sections, apply supported relocations while writable | Unsupported relocation, overflow and out-of-range `R_X86_64_32S` fail without executable mapping |
| W^X transition | Target: unmap writable aliases; map code read/execute, read-only data read-only and writable data non-executable | **Blocked in current x86_64 profile:** no execute-disable page control is exposed by the pinned mapping API; no execution claim is allowed |
| Capability grant | Bind only manifest-authorised endpoints, BAR frames, IRQ notifications and DMA leases through existing services | Module cannot acquire raw PCI config, `IRQControl`, other driver resources or arbitrary frames |
| Entry | Invoke resolved `init_module()` only in module domain after all previous stages report success | Intentional module fault is delivered to module fault endpoint and cannot crash root/service domains |
| Exit | Invoke `cleanup_module()` under controlled stop protocol, unmap all pages, revoke CSpace capabilities and invalidate generation token | Reload cannot observe old bytes/capabilities; stale IPC is rejected |

## Current x86_64 W^X blocker

The pinned x86_64 seL4 API maps pages with `seL4_CapRights_t` containing grant, read and write bits; `seL4_X86_VMAttributes` in this build exposes cache attributes only. It does not expose a separate execute-disable/NX mapping control. Therefore the current profile can create a read-only post-relocation mapping but cannot yet produce the required **independent NX proof** for writable data through this API surface. No native module execution may be claimed until SeLinOS adopts and tests an architecture-specific enforceable W^X mechanism or an explicitly justified alternative isolation design.

## Capability manifest

The manifest is an input to policy, not a set of requests that module code can self-authorise. The first real fixture has no approved device capability: it only requires the curated KABI symbols `_printk`, `__x86_return_thunk` and `module_layout`. A device module would require a separate immutable record with a PCI BDF policy identity, BAR ranges, IRQ policy, DMA-mask ceiling and service-issued device token. `deviced`, `irqd` and `dmad` remain the authorities that decide whether each grant exists.

## Entrypoint ABI boundary

`init_module` and `cleanup_module` are Linux module ABI entrypoints, but their direct invocation cannot by itself provide Linux kernel semantics. Before execution, every resolved import must be implemented by the SeLinOS version-pinned KAPI profile and have a tested calling convention. The current three-symbol fixture resolver uses placeholder non-null addresses solely to validate relocation; those values must never be used as execution targets.

## Completion criteria

A native execution claim requires one artefact with a trusted package, a complete resolved-import report, W^X mapping evidence, capability manifest evidence, an `init_module()` success marker, a `cleanup_module()` success marker and negative fault/reload tests. It must be reported as compatibility with the exact Linux 6.18.44 configuration and module fixture, not as general Linux module compatibility.
