# Phase 57: self-rooted minimal CSpace for a non-executing task M0 gate

## Status

**Status: design and implementation gate.** This default-OFF x86_64/PC99 QEMU TCG profile extends Phase 56 with the minimum self-rooted CSpace shape necessary for a target task to resolve its own CNode if it were later given execution. Root inserts exactly two capabilities into the final 16-slot CNode before configuration: a self-CNode capability in slot `0` and one notification capability in slot `1`. Root then assigns the PML4 ASID, configures the TCB with this CNode/PML4, and moves the final TCB/CNode/PML4 caps to taskd. The TCB remains without register state, IPC buffer, fault endpoint, scheduler configuration or `Resume`.

> A self CNode capability is authority over the target’s two-slot CSpace state, not authority to run. The target is never resumed, never invokes either slot, and has no capability to any device, memory frame, page table, IRQ, DMA, filesystem, network, package database or process-management service.

## Contract

| Stage | Root operation | Required result | Explicitly forbidden |
|---|---|---|---|
| Generation 1 rollback | Allocate and reverse-free one TCB/CNode/PML4 bundle before CNode population, ASID, configuration or delegation. | Bounded pre-delegation cleanup. | Any cap insertion, configuration, transfer or execution. |
| Generation 2 CSpace shape | Allocate final TCB/CNode/PML4 and notification. Invoke exactly two `seL4_CNode_Copy` operations into the final CNode: its CNode cap at slot `0`, notification cap at slot `1`, both at the CNode’s four-bit radix depth. | The final CNode holds exactly a self-root and a notification capability. | Any other final CNode copy/mint/move/delete/revoke, target invocation or source-cap move. |
| Association | Assign final PML4 ASID and configure final TCB with final CNode/PML4 and null fault endpoint/IPC buffer. | The target’s CSpace/VSpace association exists but stays non-runnable. | Registers, IPC buffer, fault endpoint, scheduling, mapping, W^X, ELF load or resume. |
| Ownership delivery | Move final TCB/CNode/PML4 caps to taskd fixed slots, then status-witness one valid query and reject duplicate. | Probe receives only endpoint/notification, no target cap. | Root final-cap copy retention, client authority or process API. |

Generation-1 pre-population cleanup is the only rollback property. Any generation-2 failure after CNode population fails closed before delegation and does not claim cleanup, ASID recycle or resource reclaim.

## Required runtime markers

```text
SeLinOS taskd self-rooted CSpace M0: root rolled back rejected TCB/CNode/PML4 generation 1 before CSpace population.
SeLinOS taskd self-rooted CSpace M0: final CNode self cap slot 0 and notification cap slot 1 inserted.
SeLinOS taskd self-rooted CSpace M0: generation 2 PML4 ASID assigned and self-rooted CSpace configured.
SeLinOS taskd self-rooted CSpace M0: final caps moved into taskd ownership slots; no registers or resume.
SeLinOS taskd self-rooted CSpace M0: self-rooted CNode ownership query passed; configured TCB remains non-executing.
```

## Promotion evidence

A SHA-bound record must bind source, transcript and images. An independent verifier must require exactly two final-CNode copies with slots `0` and `1` at radix depth `4`, copy-before-ASID-before-Configure-before-move order, and absence of target invocation, registers, resume, scheduler, mapping, ELF and Linux process primitives.

## Explicit exclusions

This gate does not establish an executable initial CSpace ABI, IPC buffer, target service invocation, fault handling, scheduler/process lifecycle, CNode reclaim, VSpace mapping, W^X, ELF loading, `clone`, `fork`, `vfork`, `pthread`, PID/TID, TLS, signals, filesystem persistence, storage, network, package trust, `dpkg`, `apt`, Linux driver execution or Debian/Ubuntu package compatibility.
