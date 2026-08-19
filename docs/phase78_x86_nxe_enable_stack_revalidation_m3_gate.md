# Phase 78: x86 NXE enable and fresh-stack revalidation M3 gate

## Status

**Status: VERIFIED — isolated QEMU TCG evidence captured on the current x86_64/PC99 build.** The prior Phase 78 M2 diagnostic observed `FSR=0x0c` for the first fresh-target `mov rax,[rsp]` attempt: a user, reserved-bit classification compatible with an execute-disable leaf being interpreted while NXE was inactive. M3 applies the minimal boot-time NXE correction and proves the accepted post-read terminal branch without reply, repair, retry, or a second resume.

## Implemented correction boundary

The correction is confined to `src/kernel/src/arch/x86/64/head.S`.

| Step | Implemented behavior | Fail-closed behavior |
|---|---|---|
| 1 | `nxe_check` verifies extended CPUID leaf availability and CPUID `0x80000001` EDX bit 20 (NX). | The kernel emits its NX diagnostic and halts before paging if NX is absent. |
| 2 | The existing EFER read-modify-write sets LME bit 8 and NXE bit 11 before paging activation. | There is no fallback that enables execute-disable leaves while NXE remains unset. |
| 3 | Existing EFER bits remain preserved by read-modify-write. | No unrelated MSR, VSpace, reply, or mapping-policy change is made. |
| 4 | The dedicated default-OFF `SeLinFreshTargetNxeStackRevalidationProbe` reuses the inherited fresh target, stack map, payload and exactly one resume. | No fault reply, repair, remap, target retry, or second resume is permitted. |

## Verified M3 observation

The isolated M3 profile reaches the accepted first branch:

> `mov rax,[rsp]` completes once and the next deliberate `UD2` produces the exact terminal post-read `UserException` witness.

The independent verifier SHA-binds the M3 kernel image, root image, transcript, NXE boot assembly, M3 protocol and root control path. It rejects reserved-bit VMFaults, bootstrap failure, a reply, repair, retry, or second resume. The result therefore proves **one bounded initial-stack read only**. It does not prove a general user stack, write access, function call/return, process abstraction, ELF loading, Linux ABI, `dpkg`, `apt`, or Debian package compatibility.

## Phase 43 raw-FSR revalidation

The Phase 43 execute-disable witness was independently revalidated under the corrected boot path. Its first instruction fault is now required to be the exact raw protection-fault `FSR=0x15`, excluding the former reserved-bit ambiguity, before its existing bounded executable-control remap/reply sequence. This does not broaden Phase 43 into a general W^X or loader claim.

## Regression evidence

The project rebuilt all **54** isolated build profiles after the global boot correction. All current SHA-bound evidence records passed the comprehensive binding audit. The full standalone verifier runner completed **87** self-contained verifiers successfully; its two explicitly skipped utilities require dedicated positional inputs and remain covered by their own evidence workflows.

## Non-claims

M3 does not prove general page-table inspection, stack contents, stack write access, ABI entry state, executable mappings beyond the bounded witnesses, ELF loading, dynamic linking, a Linux syscall surface, `dpkg`, `apt`, Debian packages, a Debian primary environment, DMA containment, or IOMMU functionality.

## Reference

[1]: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html "Intel® 64 and IA-32 Architectures Software Developer’s Manual"
