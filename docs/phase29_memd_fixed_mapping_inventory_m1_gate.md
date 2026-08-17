# Phase 29: memd fixed mapping inventory M1 gate

## Status

**Status: verified on default x86_64/PC99 QEMU TCG image `14545b3357609da1cfc592dfcfcc36865cce2ad12e39588972bcc502aab3b59d`.** Memd M1 proves only that an isolated memd domain owns and transitions a one-slot reservation state for a future fixed mapping plan. It does not create a VSpace, page table, frame, stack, mapping, reservation object or capability lease. It makes no executable mapping or W^X claim and does not change the x86 execute-disable blocker.

> **A reserved mapping-plan name is not a mapping.** M1 is not memory allocation, address-space construction, ELF loading, task creation, capability transfer or `clone` semantics.

## Exact protocol

The dedicated memd M1 probe owns one endpoint and performs an exact two-call sequence. The first `RESERVE_FIXED_PLAN(1)` must return `RESERVED`; the repeated call must return `EBUSY`. Any other label, length, opcode or slot returns a fixed invalid status without changing state. There is no client-selected address, size, rights, frame, image, stack, page-table or generation field.

| Request | Exact message words | Required reply | State transition |
|---|---|---|---|
| First fixed-plan reservation | `(SELINOS_MEMD_M1_RESERVE_FIXED_PLAN, slot=1)` | `SELINOS_MEMD_M1_RESERVED` | `FREE → RESERVED` |
| Duplicate reservation | `(SELINOS_MEMD_M1_RESERVE_FIXED_PLAN, slot=1)` | `SELINOS_MEMD_M1_EBUSY` | `RESERVED → RESERVED` |
| Any other shape | Any different label, length, opcode or slot | `SELINOS_MEMD_M1_EINVAL` | No transition |

The probe succeeds only after the ordered `RESERVED`, then `EBUSY` results. Memd emits its proof marker only after that duplicate refusal. The state remains `RESERVED`; release, VSpace creation, mapping and reuse require later independent gates.

## Capability topology

Root creates one endpoint and copies it only to memd and the memd M1 probe in fixed verified CSpace slots. Memd receives no VKA, untyped, VSpace, page-table, frame, taskd TCB, device, IRQ, DMA, PCI, IO port or IOSpace capability. Root retains all allocator and mapping authority. No endpoint from objectd, taskd, ROMFS, Linux ABI or driver paths is reused.

| Domain | Receives | Must not receive |
|---|---|---|
| memd | One client endpoint. | VKA, untyped, VSpace, page-table/frame cap, stack/image mapping, task cap, device/IRQ/DMA/PCI/I/O authority. |
| memd M1 probe | One client endpoint. | Mapping-plan authority beyond its requests, mapping/task/allocator/device capabilities. |
| Root | Construction-time endpoint and standard bootstrap references. | Runtime participation in memd reservation transitions. |

## Required evidence

The generic memd placeholder must be replaced by a dedicated M1 server and a dedicated M1 probe image. The independent verifier must bind the image, root wiring, memd source, probe, protocol header and QEMU log by SHA-256. It must require ordered reserve/duplicate-refusal/probe-success markers and reject source paths containing `vka_`, `vspace_`, `sel4utils_configure_process`, `seL4_Untyped_Retype`, `seL4_TCB_`, device/IRQ/DMA/PCI/I/O operations or Linux clone dispatch.

## Promotion criteria

M1 is verified: `verify_memd_fixed_mapping_inventory_m1.py` passed against the SHA-bound QEMU log `selinos_memd_fixed_mapping_inventory_m1.boot.log`, and the complete default-plus-opt-in suite passed with 35 independent verifiers. The compatibility matrix claims only “memd has a one-slot fixed-mapping-plan reservation-state proof with duplicate refusal.” It retains absence of VSpace/frame/stack/image allocation, mapping, W^X/executable behavior, cap lease, dynamic task, clone, fork, pthread, teardown, glibc, `dpkg` and `apt` claims.
