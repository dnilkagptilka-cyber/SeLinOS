# Phase 53 VSpace-root research notes

The next task-resource milestone must remain narrower than process creation. On x86_64, the top-level seL4 VSpace object is a `seL4_PML4`; intermediate `PDPT`, page-directory and page-table objects are separately required before any page mapping can succeed.[1] The current Phase 53 candidate therefore concerns only allocation, rollback and capability placement of an **inert PML4 root**, not an address-space layout, ASID assignment, page-table construction, mapping or execution.

> seL4 leaves virtual-memory management policy to user level beyond the kernel primitives that manipulate paging structures.[1]

The source-audited helper for this platform is `vka_alloc_vspace_root(vka, &object)`. It resolves to the local architecture's PML4 allocation wrapper. The candidate rollback path must call `vka_free_object` while the newly allocated root is still held by the root VKA and before any transfer. It must then allocate a replacement root and move—not copy—its capability into taskd's fixed CSpace slot. The taskd witness must not invoke the final PML4, associate it with a TCB, assign an ASID or map any intermediate object or frame.

| Bound | Candidate Phase 53 M0 behavior | Not established |
|---|---|---|
| Object | One generation-1 VSpace-root allocation is rolled back; one generation-2 PML4 root is allocated and moved to taskd. | An active address space, ASID assignment, task linkage or page-table hierarchy. |
| Authority | The final root cap exists only in documented taskd CSpace location; probe receives status only. | Reparenting, post-delegation reclamation, cross-domain authority or root-cap fan-out. |
| Memory | No intermediate paging object or frame is created, mapped or unmapped. | Virtual address layout, W^X mappings, ELF `PT_LOAD`, dynamic linker or process launch. |
| Evidence | QEMU TCG marker sequence plus SHA-bound source/image/log record and an independent verifier. | Linux ABI, `clone`, filesystem, networking, `dpkg` or `apt`. |

Untyped memory is provided to the initial root and retyped into kernel objects through explicit user-level authority.[2] That makes ordering visible and reviewable: the generation-1 cleanup must precede delegation, but it is not a general proof of recovering arbitrary descendant caps. The Phase 53 gate must use exactly this limited interpretation.

## References

[1]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
[2]: https://docs.sel4.systems/Tutorials/untyped.html "seL4 Untyped tutorial"
