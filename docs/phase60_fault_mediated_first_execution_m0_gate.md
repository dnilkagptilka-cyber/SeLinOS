# Phase 60: fault-mediated first-execution M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinTaskdFaultWitnessProbe=ON` profile begins from the Phase 59 isolated target construction and adds a dedicated badged fault endpoint in target CSpace slot 3. Root configured the target with the slot-3 fault endpoint, requested the Phase 59 whole-context zero register state, verified zero-plus-normalized-`rflags=0x202` read-back, then called `seL4_TCB_Resume` exactly once.

Root received exactly one x86_64 `seL4_Fault_VMFault` on its retained fault endpoint. The reported instruction pointer and fault address were both zero; `PrefetchFault` identified the instruction-fetch failure. Root deliberately did **not** call `seL4_Reply` and issued no second resume, leaving the target fault-blocked. The SHA-bound QEMU TCG transcript, independent verifier, 32-profile rebuild, and 66-standalone-verifier regression passed.

> A fault delivery after one resume proves only a bounded dispatch-to-fault transition. It is not proof that the target completed an instruction or that SeLinOS has a general runnable process, ELF execution environment, or Linux ABI.

## Required transaction

1. Reproduce Phase 59 generation-1 TCB/CNode/PML4/frame rollback and generation-2 isolated construction.
2. Copy exactly four target CSpace capabilities: self-CNode at slot 0, notification at slot 1, IPC-buffer frame at slot 2, and a badged fault endpoint at slot 3.
3. Assign the target PML4 ASID, map the ordinary IPC page at `0x70000000`, and configure the TCB with the slot-3 fault endpoint.
4. Write and read the full zero-request context, requiring only normalized `rflags=0x202` as the sole nonzero read-back word.
5. Perform exactly one `seL4_TCB_Resume`, receive exactly one VM fault with IP/address zero and instruction-prefetch indication, then withhold any fault reply.
6. Move final TCB/CNode/PML4 caps to the status-only taskd witness. The probe must observe exactly one owned response then one rejection.

## Forbidden scope

The implementation must not set an entry point, stack pointer, TLS base, nonzero general register, priority, scheduling parameter, executable page mapping, ELF image, user instruction bytes, or user-stack page. It must not call `seL4_Reply` for the target fault, issue a second `seL4_TCB_Resume`, map/repair the faulting address, invoke target IPC, or make any claim of successful instruction completion, general task launch, Linux thread/process behavior, driver support, `dpkg`, or `apt` compatibility.

## References

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 Fault handling tutorial"
[2]: https://docs.sel4.systems/Tutorials/threads.html "seL4 Threads tutorial"
