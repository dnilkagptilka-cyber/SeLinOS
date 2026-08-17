# Phase 55: configured but non-executing task linkage M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdSuspendedLinkageProbe=ON` advances from Phase 54's mutually unlinked resource bundle to exactly one **configured but non-executing** TCB. Root allocates generation-1 TCB/CNode/PML4 objects and releases them before any delegation. It then allocates generation 2, assigns an ASID to the PML4, invokes `seL4_TCB_Configure` to associate the final TCB with that final CNode and PML4, and moves the three configured-resource caps—not copies—to fixed taskd CSpace slots.

> A configured TCB is not an executing task. The final TCB receives no IPC buffer, fault endpoint, register context, scheduling configuration or resume invocation. Its CNode remains empty, its PML4 has no intermediate paging structures or frame mappings, and the profile must never invoke `seL4_TCB_WriteRegisters` or `seL4_TCB_Resume`.

## Contract

| Stage | Root operation | Required result | Explicitly forbidden |
|---|---|---|---|
| Generation 1 rollback | Allocate TCB, 16-slot CNode and x86_64 PML4; deliberately reject only after the three allocations; free PML4, CNode and TCB in reverse order. | The pre-configuration cleanup branch runs before ASID assignment, TCB configuration or cap move. | Partial delegation, post-configuration rollback, execution or a recovery claim for arbitrary failures. |
| Generation 2 allocation | Allocate replacement TCB/CNode/PML4 in root. | The root owns every source cap during ASID and configuration calls. | Probe access or taskd authority prior to configuration. |
| Association | Invoke `seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, owned_vspace_root.cptr)` then `seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr, 0, owned_vspace_root.cptr, 0, 0, seL4_CapNull)`. | The final TCB receives one CSpace/VSpace association while it remains non-runnable. | IPC-buffer frame, fault handler, page-table/frame allocation, mapping, registers, priority/scheduling setup, resume or execution. |
| Ownership delivery | Move final TCB, CNode and PML4 caps to taskd slots 9, 10 and 11; taskd returns one status witness then rejects duplicate. | Root verifies exact destination slots; probe has only endpoint and completion notification. | Final-cap copy, root retained final-cap copy, client control or general process manager API. |

The deliberate generation-1 rollback is the only rollback property established. If an ASID-assignment or configuration invocation fails for generation 2, the profile fails closed before cap delegation; it does **not** claim post-configuration teardown, ASID recycling, rollback or object reclamation.

## Required runtime markers

```text
SeLinOS taskd suspended linkage M0: root rolled back rejected TCB/CNode/PML4 generation 1 before configuration.
SeLinOS taskd suspended linkage M0: generation 2 PML4 ASID assigned and TCB CSpace/VSpace configured.
SeLinOS taskd suspended linkage M0: configured resource caps moved into taskd ownership slots; no registers or resume.
SeLinOS taskd suspended linkage M0: linkage ownership query passed; configured TCB remains non-executing.
```

## Verified evidence

The clean QEMU TCG transcript records generation-1 pre-configuration rollback, generation-2 PML4 ASID assignment, TCB CSpace/VSpace configuration, move-only delivery of configured resource caps to taskd, a non-invoking taskd witness and probe success. `tests/artifacts/selinos_taskd_suspended_linkage_m0.verification.json` binds transcript, images, protocol, server, probe, root wiring, CMake gate and this document by SHA-256. `tools/verify_taskd_suspended_linkage_m0.py` independently enforces ASID-before-Configure-before-move order and rejects register, resume, scheduler and mapping primitives. Promotion completed a successful all-profile rebuild (**27 / 27**) and fresh standalone regression (**61 / 61**).

## Promotion evidence

A SHA-bound record must include protocol, server, probe, root wiring, CMake gate, generated images, QEMU transcript and this gate. The independent verifier must require the exact generation-1 allocation/free ordering, generation-2 ASID assignment before TCB configuration before every final-cap move, fixed destination slots and the absence of register, resume, scheduler, page-table/frame, mapping, CNode population and Linux process primitives.

## Explicit exclusions

This gate does not establish a running task, task start/stop/reuse, post-configuration rollback, ASID recycle/reclaim, CNode population, IPC buffer, fault delivery, page-table or frame lifecycle, mappings, W^X, ELF loading, scheduler policy, `clone`, `fork`, `vfork`, `pthread`, PID/TID, TLS, signals, wait/reap, filesystem persistence, storage, networking, package trust, `dpkg`, `apt`, Linux driver execution or Debian/Ubuntu package compatibility.
