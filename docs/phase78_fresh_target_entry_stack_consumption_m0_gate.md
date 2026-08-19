# Phase 78: fresh target entry/stack consumption M0 gate

## Status

**Status: BLOCKED — FAIL-CLOSED, NOT IMPLEMENTED OR VERIFIED.** Phase 77 proved one RIP-only `UserException` repair and a second terminal invalid-opcode fault on the fresh target. It did not initialize or consume the fresh stack frame: the fixed `rsp=0x70002ff8` was only preserved across exception delivery. A temporary Phase 78 experiment materialized `ret; ud2`, initialized a root-private stack word and requested a fresh NX stack map, but QEMU TCG observed a `VMFault` on the target’s data read at `0x70002ff8`. The experiment was removed from the source tree without a commit. No entry/stack-consumption or Linux ABI claim exists. Phase 78 may resume only after a fresh-VSpace page-table translation audit identifies and proves the required mapping path.

> A one-word `ret` witness proves only that one initialized 64-bit word at the fixed stack pointer was consumed into the instruction pointer. It does not establish an AMD64 process entry stack, `argc`/`argv`/`envp`/auxv, a call chain, red-zone semantics, unwinding, signals, a C runtime, or Linux compatibility.

| Step | Required action | Required boundary |
|---:|---|---|
| 1 | Complete the verified fresh-bundle allocation, configuration, ASID, IPC mapping and fixed full-context provenance chain. | No reuse, dynamic allocator, general process or Linux ABI claim. |
| 2 | Through separate temporary root-private aliases, write the three-byte `ret; ud2` sequence at entry `0x60000000` and exactly one little-endian 64-bit stack word `0x60000001` at `rsp=0x70002ff8`. Remove both aliases before target mappings. | No ELF, loader, target writable-executable page, external stack source or unbounded stack content. |
| 3 | Map entry RX and stack NX at their fixed target addresses, resume once, and receive one badged `UserException`. | No reply, no second resume, no scheduler/process claim. |
| 4 | Validate `FaultIP=0x60000001`, `SP=0x70003000` and invalid-opcode vector `6`. | The stack-pointer increment must be exactly eight bytes; no subsequent continuation, fault repair, teardown or reuse. |

## Required implementation controls

The Phase 78 profile must be default-OFF and independent. Its protocol must bind the entry virtual address, terminal `UD2` virtual address, initial stack pointer, post-`ret` stack pointer, invalid-opcode vector, badge, payload length and one-word stack record. The root must use no `seL4_TCB_WriteRegisters` after the Phase 75 context write/readback, no `seL4_Reply`, and exactly one `seL4_TCB_Resume(fresh_tcb.cptr)` in the combined fresh-target transaction. The target stack mapping must retain x86 `ExecuteDisable`; the entry mapping must remain executable only after the root write alias is removed.

An independent verifier must hash-bind the isolated images, transcript, gate, protocol, CMake option and root source. It must inspect the Phase 78 block independently from historical blocks and require exactly one root alias for each frame, explicit little-endian stack-word writes, alias removal before target maps, RX entry/NX stack mapping, one resume and the observed post-`ret` `FaultIP`/`SP` pair. It must reject reply, second resume, a target-stack execute mapping, a writable-executable target page, an ELF path and any Linux ABI claim.

## Explicit non-claims

Phase 78 would not prove a general ABI entry stack, a function call/return convention, arbitrary stack access, signal frames, TLS, C runtime initialization, ELF loading, dynamic linking, process creation, Linux ABI, `dpkg`, or `apt`.

## Static translation audit after fail-closed rollback

The failed experiment is not retained as an implementation claim, but its mapping assumptions were audited against the pinned source. The fresh IPC virtual address `0x70000000`, proposed stack base `0x70002000`, and proposed initial `rsp=0x70002ff8` share x86_64 indices `PML4=0`, `PDPT=1`, `PD=384`; the stack word is at `PT=2`, page offset `4088`. The proposed entry `0x60000000` instead uses `PML4=0`, `PDPT=1`, `PD=256`, `PT=0`.

`sel4utils_map_page_with_attributes` first requests a leaf map and, on `seL4_FailedLookup`, allocates/maps the reported paging object and retries until the leaf map succeeds. Its attribute-aware variant changes only the supplied leaf attributes. The pinned x86_64 kernel’s `makeUserPTE` creates a present, user-accessible leaf PTE; `ExecuteDisable` controls only its `xd` bit. Therefore a successful return from that helper should correspond to a present user NX mapping, not an intentionally unreadable page.

The temporary root alias was also not a justified explanation: `VSPACE_PRESERVE` prevents freeing the backing object but `sel4utils_unmap_pages` invokes `seL4_ARCH_Page_Unmap` for the alias capability before preserving it. The observed QEMU VMFault on the target’s data read at `0x70002ff8` consequently remains an unresolved discrepancy between the nominal map success and the target translation. Before Phase 78 is retried, the next design must add a narrowly scoped mapping-provenance witness that records each page-map result and the relevant fresh-VSpace hierarchy ownership without asserting a successful `ret` or process stack.
