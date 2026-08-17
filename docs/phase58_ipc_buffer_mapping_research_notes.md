# Phase 58 IPC-buffer mapping research notes

An seL4 IPC buffer must be backed by a mapped, non-device frame and is configured separately from a TCB's CSpace/VSpace association. The official seL4 documentation describes allocating and mapping a frame for the IPC buffer before configuring the TCB, and notes alignment and page-boundary constraints for the buffer location.[1] [2]

Phase 58 will therefore isolate exactly one normal 4 KiB frame, one fixed target virtual address, and the minimum intermediate paging objects needed to map it into the already ASID-assigned target PML4. It must retain a zeroed but unexecuted register state and exclude instruction frames, stacks, ELF segments, target IPC invocation, TCB resume and scheduler policy.

## References

[1]: https://docs.sel4.systems/Tutorials/libraries-2.html "seL4 Libraries: IPC tutorial"
[2]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
