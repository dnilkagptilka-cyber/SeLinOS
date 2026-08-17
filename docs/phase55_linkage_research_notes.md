# Phase 55 non-executing task-linkage research notes

Phase 55 is the first gate that would connect separately allocated task resources. A seL4 TCB carries CSpace and VSpace capabilities, and the scheduler considers a TCB runnable only once it is resumed and not blocked.[1] A successful configuration without register writes or a resume therefore has a useful, narrow safety interpretation: the resource association exists, while no user instruction can execute.

The local `libsel4utils` code assigns an ASID with `seL4_ARCH_ASIDPool_Assign` before using a VSpace root; on x86 this dispatches to `seL4_X86_ASIDPool_Assign`.[2] The local `api_tcb_configure` wrapper calls `seL4_TCB_Configure` for the non-MCS configuration used by this SeLinOS build.[3] The planned gate must invoke these primitives only on root-held temporary capabilities **before** ownership transfer into taskd; taskd receives the configured but suspended final TCB, CNode and PML4 only after root validates their exact destination slots.

| Required Phase 55 property | Source-grounded constraint | Explicit exclusion |
|---|---|---|
| ASID association | Root invokes `seL4_X86_ASIDPool_Assign` on the final PML4 before TCB configuration. | ASID reuse/revoke/reclaim and any mapping. |
| Resource linkage | Root invokes `seL4_TCB_Configure` with final CNode and PML4 caps and a null fault endpoint / IPC buffer. | IPC-buffer frame, fault endpoint, CNode contents, page tables, frames and memory mappings. |
| No execution | The final TCB receives no register write and no `seL4_TCB_Resume` invocation. | Instruction execution, scheduler lifecycle, Linux process/thread semantics. |
| Ownership transfer | After successful root-side linkage, the three final caps move to fixed taskd slots. | Probe access, root-copy retention, taskd control protocol beyond status witnessing. |

> The seL4 threads tutorial treats TCB configuration, register initialization and resume as separate steps. It identifies resume as the action that makes the new TCB runnable.[1]

## References

[1]: https://docs.sel4.systems/Tutorials/threads.html "seL4 Threads tutorial"
[2]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
[3]: https://docs.sel4.systems/Tutorials/how-to-libs.html "How-to guide for the seL4 C libraries"
