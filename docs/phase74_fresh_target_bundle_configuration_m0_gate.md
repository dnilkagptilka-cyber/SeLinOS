# Phase 74: fresh target-bundle configuration M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinFreshTargetBundleConfigurationProbe=ON` profile completed the Phase 69–73 prerequisite chain, configured one fresh suspended bundle, passed isolated QEMU TCG evidence, independent verification, a **46-profile** rebuild and **80/80** standalone verifiers. It does not establish execution, ASID reuse or Linux compatibility.

**Design basis:** Phase 74 follows verified Phase 73 fresh inert-bundle construction. It may allocate one fresh root-owned fault endpoint, copy exactly the fault endpoint, self-CNode and IPC-frame capabilities into the fresh target CNode, assign the fresh PML4 to the fresh TCB through one `seL4_TCB_Configure` call, and retain the target suspended. It must not map pages, write registers, set scheduling parameters, reply, resume, execute, free any object or construct another bundle.

> A configured but suspended target is not a process. This gate proves neither initial instruction context, target execution, fault delivery, ELF loading, reusable resource semantics nor a Linux ABI boundary.

## Required transaction

| Step | Operation | Required result | Stop rule |
|---:|---|---|---|
| 1 | Allocate one fresh fault endpoint | Root retains non-null endpoint descriptor. | No target action before all later configuration succeeds. |
| 2 | Populate fresh CNode slots 0–2 | Copy self CNode, IPC frame and badged fault endpoint only. | No entry/stack copy, mint outside fault badge, or additional cap. |
| 3 | Assign fresh PML4 to the root ASID pool and map IPC frame | Assign exactly one fresh PML4 ASID, then map exactly one IPC page at fixed IPC virtual address. | No ASID reuse, entry/stack mapping or executable page. |
| 4 | Configure fresh TCB | Use fresh CNode, PML4, badged endpoint and IPC address. | No register write, scheduler configuration, reply or resume. |
| 5 | Observe retained suspension | Log one bounded root-held configuration record. | No execution or successor construction. |

## Explicit non-claims

Phase 74 does not prove ASID reuse, an executable mapping, entry/stack provenance, register state, first instruction fetch, fault handling, target execution, process lifecycle, ELF, Linux ABI, `dpkg`, or `apt`.
