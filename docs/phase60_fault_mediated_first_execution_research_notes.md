# Phase 60: fault-mediated first-execution M0 — research notes

## Objective

Phase 60 must be smaller than a process-launch claim. It will prove exactly one controlled transition from the Phase 59 suspended target into a kernel-delivered x86_64 VM fault, then deliberately leave the target blocked by withholding a fault reply. The target receives no executable mapping, entry point, stack, TLS base, ELF image, scheduler-policy change, or fault-reply-based continuation.

## Pinned local ABI

The Phase 59 x86_64 user context has 20 words. Phase 60 will make the same all-zero write request with `resume_target=0u`; pinned seL4 sanitization is expected to read back `rflags=0x202` and zeros elsewhere. The local x86_64 VM-fault ABI labels message registers in this order: `seL4_VMFault_IP`, `seL4_VMFault_Addr`, `seL4_VMFault_PrefetchFault`, and `seL4_VMFault_FSR`.

The target CSpace will contain its self-CNode, a notification, its IPC-buffer frame, and a badged fault-endpoint capability. The `seL4_TCB_Configure` fault-endpoint argument resolves to that target-CNode slot. Root retains the receive capability and uses it to receive exactly one fault after its one explicit `seL4_TCB_Resume` call. Root does **not** call `seL4_Reply`, so the target remains fault-blocked.

> Receiving a VM fault proves a controlled first execution attempt was dispatched far enough to fault. It does not prove a successfully executed instruction, an ELF runtime, a usable stack, a process lifecycle, or Linux compatibility.

## External references

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 Fault handling tutorial"
[2]: https://docs.sel4.systems/Tutorials/threads.html "seL4 Threads tutorial"
[3]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API Reference"

## Pinned project sources

[4]: `src/kernel/libsel4/sel4_arch_include/x86_64/sel4/sel4_arch/constants.h` — x86_64 VM-fault message indices
[5]: `src/kernel/libsel4/sel4_arch_include/x86_64/sel4/sel4_arch/types.h` — x86_64 `seL4_UserContext`
[6]: `src/kernel/src/arch/x86/machine/registerset.c` — RFLAGS sanitization
