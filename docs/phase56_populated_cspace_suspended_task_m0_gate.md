# Phase 56: minimally populated CSpace for a non-executing task M0 gate

## Status

**Status: verified bounded M0 proof.** The default-OFF x86_64/PC99 QEMU TCG profile `SeLinTaskdPopulatedCspaceProbe=ON` extends Phase 55 by proving only that root can place one explicit notification capability in slot `1` of the final task CNode **before** that CNode becomes the configured TCB CSpace. The final PML4 receives an ASID, the TCB is configured with the populated CNode and PML4, and the three final resource caps move—not copy—to taskd. The TCB receives no register state, IPC buffer, fault endpoint, scheduler configuration or resume.

> Slot `1` is a notification capability held exclusively inside the final CNode. It is not bound to the target TCB, is never signalled or waited on by the target, and confers no device, memory, IRQ, DMA, filesystem, network or process-management authority. The target remains non-executing.

## Contract

| Stage | Root operation | Required result | Explicitly forbidden |
|---|---|---|---|
| Generation 1 rollback | Allocate TCB, 16-slot CNode and PML4, then deliberately free PML4/CNode/TCB in reverse order before CNode population, ASID or configuration. | Pre-delegation cleanup is observed. | Capability insertion, ASID assignment, task configuration, transfer or execution. |
| Generation 2 CNode population | Allocate replacement TCB/CNode/PML4 and notification. Invoke exactly one `seL4_CNode_Copy` to put the notification cap in final CNode slot `1`. | One non-device notification cap is inserted; root retains the source notification cap as the service-side owner. | Any second final-CNode cap, `Mint`, `Move`, `Delete`, `Revoke`, target notification binding or final CNode use by probe. |
| Association | Assign the final PML4 through root ASID pool, then call `seL4_TCB_Configure` with final TCB/CNode/PML4 and null fault endpoint/IPC buffer. | The TCB is linked to its one-cap CSpace and VSpace but stays non-runnable. | Registers, IPC buffer, fault handler, priority/scheduling setup, paging/frame allocation, mappings or resume. |
| Ownership delivery | Move final TCB/CNode/PML4 caps to taskd slots `9`–`11`; taskd accepts one status query then rejects a duplicate. | Probe receives only status endpoint and success notification. | Final-cap copy, root final-cap retention, capability fan-out or a process-management API. |

The only rollback evidence is the generation-1 pre-population cleanup. Failure after generation-2 CNode insertion, ASID assignment or TCB configuration fails closed before final-cap delegation; this gate does not claim post-configuration teardown, CNode cleanup, ASID recycling or resource reclaim.

## Required runtime markers

```text
SeLinOS taskd populated CSpace M0: root rolled back rejected TCB/CNode/PML4 generation 1 before CSpace population.
SeLinOS taskd populated CSpace M0: one notification cap inserted into final CNode slot 1.
SeLinOS taskd populated CSpace M0: generation 2 PML4 ASID assigned and populated CSpace configured.
SeLinOS taskd populated CSpace M0: final caps moved into taskd ownership slots; no registers or resume.
SeLinOS taskd populated CSpace M0: populated-CNode ownership query passed; configured TCB remains non-executing.
```

## Verified evidence

The clean QEMU TCG transcript records generation-1 pre-population rollback, exactly one notification-cap insertion into final CNode slot 1 at its four-bit radix depth, generation-2 PML4 ASID assignment, populated-CNode TCB configuration, move-only delivery to taskd and a non-invoking status witness. `tests/artifacts/selinos_taskd_populated_cspace_m0.verification.json` SHA-binds transcript, images, protocol, server, probe, root wiring, CMake gate and this document. `tools/verify_taskd_populated_cspace_m0.py` independently enforces one-copy placement and copy-before-ASID-before-Configure-before-move order. Promotion completed **28 / 28** profile builds and **62 / 62** fresh standalone verifiers.

## Promotion evidence

The evidence record must bind protocol, server, probe, root wiring, CMake gate, images, QEMU transcript and this document by SHA-256. The verifier must require exactly one final-CNode copy into slot `1`, order it before ASID and TCB configuration, require all final caps to move only after configuration, and reject target register, resume, scheduler, mapping, CNode mutation-after-copy and Linux process primitives.

## Explicit exclusions

This gate does not establish an initial CSpace ABI, bootstrap code, IPC buffer or server invocation by the target, fault delivery, scheduler/execution lifecycle, post-configuration cleanup, CNode reuse, page-table/frame lifecycle, mappings, W^X, ELF loading, `clone`, `fork`, `vfork`, `pthread`, PID/TID, TLS, signals, VFS persistence, storage, networking, package trust, `dpkg`, `apt`, Linux driver execution or Debian/Ubuntu package compatibility.
