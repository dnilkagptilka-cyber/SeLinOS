# Phase 58: isolated IPC-buffer frame mapping M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinTaskdIpcBufferProbe=ON` profile allocated one normal 4 KiB frame, copied its capability into the already self-rooted target CSpace, mapped it at `0x70000000` in the already ASID-assigned target PML4, and configured the address/frame as the suspended TCB’s IPC buffer. The address is page-aligned and therefore also **512-byte-aligned**. The target had no register context, endpoint invocation, stack, ELF mapping, or resume. The SHA-bound QEMU TCG transcript, independent verifier, 30-profile rebuild, and 64-standalone-verifier regression passed.

> A configured IPC buffer is a mapped data page, not evidence of IPC activity or target execution.

## Evidence-candidate transaction

The root first allocated and freed one rejected generation containing a TCB, CNode, PML4, and normal frame. It then allocated generation 2, inserted the self-CNode and notification capabilities, assigned an ASID, mapped the frame through the target PML4 (allocating intermediate paging objects as needed), copied the frame cap into target CSpace slot 2, invoked the equivalent `seL4_TCB_Configure` IPC-buffer configuration sequence, and moved final TCB/CNode/PML4 caps to taskd. A status-only service/probe pair emitted one owned reply followed by one rejection and signalled success. The serial transcript and SHA-bound evidence record are retained under `tests/artifacts/`.

## Boundary

**Target execution is out of scope.** This gate excludes instruction frames, stack, ELF loading, register writes, target IPC, resume, scheduler configuration, DMA, persistent storage, and all Linux process, driver, `dpkg`, or `apt` claims. The normal frame is not an executable mapping claim and creates no userspace program.

## References

[1]: https://docs.sel4.systems/Tutorials/libraries-2.html "seL4 Libraries: IPC tutorial"
[2]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
