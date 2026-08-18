# Phase 62: executable instruction-fetch witness M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinTaskdExecFetchProbe=ON` profile reused the Phase 61 target CSpace and stack provenance. Root initialized exactly three bytes in a new entry frame: `0x90 0x0f 0x0b` (`nop; ud2`). It mapped only that entry frame executable at `0x60000000`; the stack frame at `0x70002000` remained `seL4_X86_ExecuteDisable`.

Root wrote/read the full x86_64 context with `rip=0x60000000`, `rsp=0x70002ff8`, `rflags=0x202`, and all other words zero. It called `seL4_TCB_Resume` exactly once. Root received exactly one badged `seL4_Fault_UserException` whose fault IP was `0x60000001`, SP was `0x70002ff8`, and exception number was vector `6` (invalid opcode). Root withheld reply and issued no second resume; target remains blocked. The SHA-bound QEMU TCG transcript, independent verifier, 34-profile rebuild, and 68-standalone-verifier regression passed.

> A user exception at `rip+1` after the `nop; ud2` sequence proves only that one `nop` completed before the deliberate invalid-opcode fault. It is not proof of a general process launch, C runtime, ELF loading, arbitrary code execution, or Linux ABI.

## Required transaction

1. Reproduce Phase 61 isolated target construction, canonical/ABI-aligned context validation, and generation-1 rollback.
2. Allocate distinct IPC, entry, and stack frames. Copy each target cap to fixed CSpace slots.
3. Initialize root-only entry-frame alias with exactly `nop; ud2`, then unmap that alias before target resume.
4. Map entry with default executable attributes; map stack with `seL4_X86_ExecuteDisable`; configure the badged fault endpoint.
5. Write/read back full context, perform exactly one resume, receive one post-`nop` user-exception record, and withhold reply.
6. Transfer only final TCB/CNode/PML4 caps to a status-only taskd witness. Probe observes exactly one owned result and one rejection.

## Forbidden scope

No second executable page, instructions other than `nop; ud2`, executable stack mapping, return address, argument block, initial stack image, ELF/program header, loader, syscall, IPC invocation by target, fault reply, second resume, continuation, cleanup/reuse lifecycle, general task/process claim, Linux thread/process behavior, driver support, `dpkg`, or `apt` compatibility is permitted.

## References

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 Fault handling tutorial"
[2]: https://www.felixcloutier.com/x86/ud "UD — Undefined Instruction"
