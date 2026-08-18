# Phase 61: entry-point and stack provenance M0 — research notes

## Scope

Phase 61 is a register-and-mapping provenance gate, not a runnable-program gate. It will allocate two distinct ordinary 4 KiB frames: an **entry candidate** mapped at fixed user virtual address `0x60000000`, and a **stack frame** mapped at `0x70002000`. Both leaf mappings will carry the x86 `seL4_X86_ExecuteDisable` attribute. The target will not be resumed in this phase.

The target context will request `rip=0x60000000`, `rsp=0x70002ff8`, other general words zero, and the Phase 59 kernel-normalized `rflags=0x202`. `0x70002ff8` lies in the mapped stack page and has `rsp mod 16 = 8`, satisfying the AMD64 System V function-entry relation that `(%rsp - 8)` is 16-byte aligned. Both fixed addresses are low canonical x86_64 user addresses.

## Provenance ledger

The target CSpace will receive its self-CNode, notification, IPC-buffer frame, badged fault endpoint, entry frame, and stack frame. Root will assign an ASID, map the IPC, entry, and stack frames before writing the complete context, read all 20 words back, and reject any mismatch. No code bytes will be written, no executable leaf mapping will be created, no return sentinel/argument/ELF/stack image will be supplied, and no `seL4_TCB_Resume` or fault reply will occur.

> A canonical, aligned, read-back entry/stack context with NX-provenance mappings is not instruction execution. It establishes only prerequisites for a later separately gated instruction-fetch experiment.

## References

[1]: https://refspecs.linuxbase.org/elf/x86_64-abi-0.98.pdf "System V Application Binary Interface — AMD64 Architecture Processor Supplement"
[2]: https://docs.sel4.systems/Tutorials/threads.html "seL4 Threads tutorial"
[3]: `src/projects/helixos/src/x86_nx_mapping_probe.c` — verified local `seL4_X86_ExecuteDisable` mapping pattern
[4]: `src/kernel/libsel4/sel4_arch_include/x86_64/sel4/sel4_arch/types.h` — pinned local register context
