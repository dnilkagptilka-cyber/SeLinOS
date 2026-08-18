# Phase 61: entry-point and stack provenance M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinTaskdEntryStackProbe=ON` profile starts from the Phase 60 isolated target topology and does not resume the target. Root allocated distinct ordinary frames for an entry candidate and a stack, mapped both with `seL4_X86_ExecuteDisable`, and wrote/read a full 20-word x86_64 context with `rip=0x60000000` and `rsp=0x70002ff8`.

`rip` and `rsp` are low canonical user addresses. The stack frame contains `rsp`, and `rsp mod 16 = 8`, so `rsp - 8` is 16-byte aligned under the AMD64 System V function-entry rule. Pinned seL4 sanitization returned `rflags=0x202`; the remaining requested general register words were zero. The target CSpace holds distinct self-CNode, notification, IPC-buffer, badged fault endpoint, entry-frame, and stack-frame capabilities. The SHA-bound QEMU TCG transcript, independent verifier, 33-profile rebuild, and 67-standalone-verifier regression passed.

> Mapping an entry candidate with NX and writing a canonical, ABI-aligned stack pointer proves provenance only. It does not permit or prove instruction fetch, code completion, ELF execution, a process launch, or Linux compatibility.

## Required transaction

1. Reproduce Phase 60 generation-1 rollback and generation-2 TCB/CNode/PML4/IPC/fault-endpoint construction.
2. Allocate distinct entry and stack frames, and copy each into a fixed target CSpace slot.
3. Map IPC, entry, and stack frames through the target PML4; entry and stack mappings must use `seL4_X86_ExecuteDisable`.
4. Configure the TCB with its badged target-CNode fault endpoint.
5. Write and read the full context, accepting only expected `rip`, `rsp`, and kernel-normalized `rflags=0x202`; all other checked words must be zero.
6. Transfer only final TCB/CNode/PML4 caps to the status-only taskd witness. The probe must observe one owned result followed by one rejection.

## Forbidden scope

The implementation must not write instruction bytes, map an executable leaf, map an ELF or loader image, place a return address/argument/initial stack image, call `seL4_TCB_Resume`, call `seL4_Reply` for the target, deliver a fault, repair a fault, or claim successful instruction execution, general process launch, Linux thread/process behavior, driver support, `dpkg`, or `apt` compatibility.

## References

[1]: https://refspecs.linuxbase.org/elf/x86_64-abi-0.98.pdf "System V Application Binary Interface — AMD64 Architecture Processor Supplement"
[2]: https://docs.sel4.systems/Tutorials/threads.html "seL4 Threads tutorial"
