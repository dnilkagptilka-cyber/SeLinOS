# Phase 50: allocator-owned dynamic task allocation prerequisite M0 gate

## Status

**Status: verified, bounded M0 prerequisite proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinTaskdDynamicAllocationProbe=ON` profile has independently evidenced one status-only reserve/reject/release/re-reserve transition with generations 1 then 2. The SHA-bound record is `tests/artifacts/selinos_taskd_dynamic_alloc_m0.verification.json` and its checker is `tools/verify_taskd_dynamic_alloc_m0.py`. This gate establishes only one taskd-owned dynamic allocation prerequisite: a bounded request may reserve one previously free resource-plan slot, observe a unique generation/ownership witness, release it, and prove that the same slot becomes reusable under the same allocator policy.

> The gate is deliberately below process creation. It may prove a local allocator lifecycle but cannot establish Linux `clone`, `fork`, `vfork`, `pthread`, PID/TID semantics, scheduling, address-space copying, TLS inheritance, signal handling, wait/reap semantics or execution of arbitrary children.

## Closed transaction

| Step | Required behavior |
|---:|---|
| 1 | Taskd receives one root-local request for a single dynamic plan slot from an explicitly bounded pool. |
| 2 | It reserves one free slot and returns only a status, slot identity and monotonically checked generation witness. |
| 3 | It rejects a duplicate concurrent reservation of the same occupied slot. |
| 4 | It releases exactly the owned reservation and accepts one fresh reservation of that slot with a distinct generation. |
| 5 | No TCB, CSpace, VSpace, page table, endpoint, thread, process image, CPU register state or user executable is constructed. |

## Acceptance and non-claims

A successful M0 can state only that the fixed allocator policy has a bounded reserve/reject/release/re-reserve transition. It cannot claim dynamic task construction, capability transfer, thread execution, process lifecycle, Linux ABI process semantics, memory safety beyond the named reservation record, or any package/runtime compatibility.

## Promotion rule

Promotion requires an independent verifier, SHA-bound QEMU trace and full regression. A later construction stage must separately prove ownership of every allocated TCB, CSpace, VSpace, IPC endpoint, frame and mapping, including failure rollback and final teardown.

## Audit finding

The existing `taskd_reservation_m1` path accepts one exact slot-1 request, forwards status-only object/memory reservations, and intentionally leaves both reservations pending. The existing `taskd_m1` path validates two generations of one **pre-constructed** child through suspend/resume. Neither has a release operation or a dynamic allocator-owned pool. Therefore Phase 50 must introduce a new isolated service/protocol topology; it must not relabel either fixed-slot proof as dynamic allocation.
