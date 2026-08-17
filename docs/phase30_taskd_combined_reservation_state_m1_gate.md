# Phase 30: taskd combined reservation-state M1 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `143edd876ade3831c1f58abba8df08c0b2b2b3bbba32762beab8a9ca1cf7aa61`.** Objectd M1 and memd M1 independently prove fixed local reservation-state protocols. Their present dedicated M1 probes consume the only reservation and intentionally provide no taskd endpoint. A combined taskd proof must therefore use new, separately named objectd/memd reservation services or a versioned dual-client protocol; it must not silently reuse, bypass or reinterpret the standalone M1 evidence.

## Single permitted scenario

A dedicated combined-reservation probe sends exactly `TASKD_RESERVE_BUNDLES(slot=1)` to a dedicated taskd M1.5 service. Taskd sends its own exact reserve request to an objectd M2 reservation service and a memd M2 reservation service. Each returns only a status word. When, and only when, both return `RESERVED`, taskd reports `BOTH_RESERVED` to the probe. No capability is transferred, no object/mapping is created, no child is configured and no task is resumed.

| Transition | Owner | Required condition | Prohibited shortcut |
|---|---|---|---|
| Client request | Combined probe → taskd | Exact opcode, slot `1`, two words. | Direct objectd/memd client access, user-supplied resource parameters. |
| Object reservation | taskd → objectd M2 | Exact `RESERVE_OBJECT_PLAN(1)` returns `RESERVED`. | Reusing objectd M1’s occupied state or taking an object cap. |
| Mapping reservation | taskd → memd M2 | Exact `RESERVE_MAPPING_PLAN(1)` returns `RESERVED`. | Reusing memd M1’s occupied state or taking a VSpace/frame/mapping cap. |
| Combined result | taskd → probe | Both reservations were observed in the same taskd request generation. | Treating one reply, stale response or marker string as success. |
| Terminal state | All three servers | `RESERVED_PENDING`; no reuse/release. | Dynamic child creation, configuration, start or teardown claim. |

## Required authority topology

The combined M1.5 bundle creates four isolated processes: taskd-combined, objectd-reserve-M2, memd-reserve-M2 and the combined probe. Root creates three new endpoints, one for each directed request/reply relationship, and copies caps only to the two participating endpoints. The preexisting Phase 25 taskd M1, Phase 28 objectd M1 and Phase 29 memd M1 bundles remain unchanged and continue to verify their own claims.

The new objectd/memd M2 servers receive only one taskd endpoint each. They retain the same negative authority constraints as M1: no VKA, untyped, VSpace, frame, task, device, IRQ, DMA, PCI, I/O or IOSpace authority. Taskd-combined receives only the probe endpoint and the two downstream endpoint caps. It receives no child TCB, object bundle, mapping bundle, allocator or device authority.

## Evidence requirements

The independent verifier must bind the image, root wiring, all three new servers, combined probe, all protocol headers and QEMU log by SHA-256. It must verify ordered markers for exact client acceptance, exact objectd reservation, exact memd reservation, taskd combined success and probe success. It must reject source occurrences of capability transfer, object allocation, VSpace mapping, TCB control, Linux clone, child configuration, start/resume, release/reuse and device authority.

## Promotion criteria

M1 is verified: `verify_taskd_combined_reservation_m1.py` passed against the SHA-bound QEMU log `selinos_taskd_combined_reservation_m1.boot.log`, and the complete default-plus-opt-in suite passed with 36 independent verifiers. The compatibility matrix claims only one coordinated **status-only** reservation pair owned by taskd. It remains strictly below a lease: there are no transferred object or mapping capabilities, no dynamic task, no child, no VSpace, no frame, no W^X, no ELF load, no cleanup, no `clone`, no fork, no pthread and no package-runtime claim.
