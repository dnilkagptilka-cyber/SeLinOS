# Phase 58: isolated IPC-buffer frame mapping M0 gate

## Status

**Status: design and implementation gate.** This default-OFF gate will allocate one normal 4 KiB frame, copy its cap into the already self-rooted target CSpace, map it at one fixed aligned target virtual address in the already ASID-assigned target PML4, and configure that address/frame as the TCB IPC buffer. The target remains suspended with no register context, no endpoint invocation and no executable mapping.

> A configured IPC buffer is a mapped data page, not evidence of IPC activity or target execution.

## Boundary

The gate must prove allocation, one target-cap copy, page-table/frame mapping and `seL4_TCB_SetIPCBuffer` (or an equivalent configuration sequence) in a strict order. It excludes instruction frames, stack, ELF loading, register writes, target IPC, resume, scheduler configuration and all Linux process claims.

## References

[1]: https://docs.sel4.systems/Tutorials/libraries-2.html "seL4 Libraries: IPC tutorial"
[2]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
