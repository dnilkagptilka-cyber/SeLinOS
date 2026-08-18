# Phase 63: VM-fault reply restart to terminal user-exception M0 gate

## Status

**Status: VERIFIED.** Phase 63 supersedes the rejected idea of replying to x86 `seL4_Fault_UserException`. Pinned x86 seL4 `Arch_handleFaultReply()` permits only `seL4_Fault_VMFault`; attempting a user-exception fault reply is unsupported. The verified gate tests exactly one supported VM-fault repair/restart transition.

Root initialized a frame with the Phase 62 `nop; ud2` bytes but initially left it **unmapped** in the target VSpace. The canonical target started at `rip=0x60000000` with the established NX stack. One explicit `seL4_TCB_Resume` yielded a badged instruction-fetch VM fault at `0x60000000`. Root mapped the already initialized entry frame executable at that address and sent one zero-label `seL4_Reply` with no message-register updates. The target restarted, fetched/completed the NOP, then produced one badged `seL4_Fault_UserException` at `rip=0x60000001` for `ud2`. Root withheld this terminal reply. The SHA-bound QEMU TCG transcript, independent verifier, 35-profile rebuild, and 69-standalone-verifier regression passed.

> This proves one x86-supported VM-fault repair/restart that reaches the existing terminal invalid-opcode witness. It does not prove generic exception repair, fault-register mutation, process continuation, an exit lifecycle, ELF runtime, a loader, or Linux compatibility.

## Required transaction

1. Reproduce the isolated target, target CSpace, context and root-only `nop; ud2` frame initialization from Phase 62, but do not map that frame into the target initially.
2. Resume exactly once and receive exactly one badged instruction-fetch VM fault at `0x60000000`.
3. Map the one entry frame executable at the faulting address and send exactly one zero-label, zero-length `seL4_Reply`.
4. Receive exactly one badged UserException fault at `0x60000001`, exception vector 6, then withhold reply.
5. Delegate only final status authority after the terminal UserException blocks the target. The probe must observe one owned result and one rejection.

## Forbidden scope

No UserException reply, reply message-register updates, second `seL4_TCB_Resume`, second reply, second target executable mapping, stack image, return address, ELF, loader, syscall, target IPC, general continuation, cleanup/reuse lifecycle, process/thread claim, Linux ABI, `dpkg`, or `apt` claim is permitted.

## References

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 Fault handling tutorial"
[2]: `src/kernel/src/arch/x86/api/faults.c` — pinned x86 fault-reply support

## Implementation checkpoint

The Phase 63 protocol, status-only taskd/probe binaries, and root-side delayed entry-frame mapping are integrated under `SeLinTaskdVmRestartProbe=ON`. Root receives the first VM fault, maps the single preinitialized entry frame, issues exactly one zero-label reply, then receives the terminal UserException without reply. The required QEMU, SHA-bound evidence, independent verifier, all-profile rebuild, standalone regression, and private Git milestone criteria have passed.
