# Phase 43: x86_64 execute-disable mapping extension gate

## Status

**Status: verified, bounded M0 proof.** The pinned baseline is seL4 commit `1326364bc9135d9445d936ebc01e38a402c1f4c6`. The local default-OFF `SeLinX86NxMappingProbe=ON` x86_64/PC99 QEMU TCG profile has independently evidenced one fixed-page execute-disable instruction fault, followed by an executable remap and one fixed return witness. The SHA-bound record is `tests/artifacts/selinos_x86_nx_mapping_m0.verification.json` and the independent checker is `tools/verify_x86_nx_mapping_m0.py`.

> This is an explicitly local, default-OFF compatibility extension on top of the pinned baseline. It is not a claim that upstream seL4 already exposes an NX selector, and it does not make Linux ELF execution safe by itself.

The baseline audit remains relevant: its x86_64 `vm_attributes` bitfield exposes only PAT, PCD and PWT cache fields, while user PTE/PDE/PDPTE constructors hard-code `xd` to zero. The verified local extension changes only the stated opt-in profile and is bounded by the proof below.

## ABI-preserving representation

The extension must retain existing cache values (`Default/WriteBack=0`, `WriteThrough=1`, `CacheDisabled=2`, `Uncacheable=3`, `WriteCombining=4`) exactly. In both x86 bitfield specifications, replace the current 61-bit padding plus three cache fields with a 60-bit padding followed by `x86ExecuteDisable`, `x86PATBit`, `x86PCDBit`, `x86PWTBit`. This places execute-disable at bit 3 while leaving PAT/PCD/PWT at bits 2/1/0.

The local public x86 type adds an orthogonal bit flag:

```c
seL4_X86_ExecuteDisable = (1u << 3)
```

It may be ORed with existing cache attributes. No existing numeric cache attribute changes.

| Mapping form | Required local x86_64 behavior | Required 32-bit x86 behavior |
|---|---|---|
| 4 KiB PTE | Set leaf `xd` from `vm_attributes_get_x86ExecuteDisable(vm_attr)`. | Reject the execute-disable bit before mapping; never silently ignore it. |
| 2 MiB PDE / 1 GiB PDPTE | Set large/huge leaf `xd` from the same bit. | Reject the execute-disable bit before mapping. |
| Non-leaf page-table hierarchy | Keep `xd=0` so an executable leaf remains possible where requested. | Existing behavior. |

## Compatibility and fail-closed requirements

Only an x86_64 opt-in kernel/user ABI profile may accept the bit. The existing normal profile retains its baseline public ABI and existing mapping behavior. Any unknown attribute bit, and the execute-disable bit on 32-bit x86, must fail with `seL4_InvalidArgument` before a mapping is created. The extension must not alter frame-cap rights, cache selectors, VSpace ownership, IOMMU behavior, mapping lifetimes or syscall invocation labels.

## Required proof sequence

The implementation may be promoted only after all of the following are separately evidenced.

1. A source verifier binds the local diff against the pinned baseline and proves cache-bit compatibility plus x86_64 leaf-only `xd` propagation.
2. A fresh x86_64/PC99 QEMU TCG probe maps one fixed test code page with execute-disable, attempts an indirect call, receives a user instruction fault on its own fault endpoint, and proves the surrounding root remains live.
3. A paired control maps the same fixed page without execute-disable and reaches a one-time execution witness. Both paths must use temporary mappings and teardown them.
4. The complete regression suite passes in the baseline default profile and all extant opt-in profiles.

## Explicit non-claims

Even a verified Phase 43 demonstrates only page-level execute-disable/execute control for a fixed probe. It does not establish W^X-safe ELF loader policy, PT_LOAD mapping, relocation, dynamic linking, ELF TLS, Linux process launch, native `.ko` execution, a package database, `dpkg` or `apt`.
