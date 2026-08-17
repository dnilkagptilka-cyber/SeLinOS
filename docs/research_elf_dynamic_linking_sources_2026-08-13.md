# External sources: ELF dynamic-linking scope for SeLinOS Phase 5

This note records authoritative external findings used only to constrain the future dynamic ELF runtime scope. It does **not** claim that SeLinOS implements dynamic linking.

The System V ABI dynamic-linking specification states that a dynamically linked executable has one `PT_INTERP` program-header entry, naming its interpreter. Dynamic linking requires forming a process image from executable and shared-object segments, applying relocations, and transferring control after those steps. Its `.dynamic` information is in a `PT_DYNAMIC` loadable segment. `DT_NEEDED` entries name dependencies through offsets into `DT_STRTAB`; `DT_STRTAB`, `DT_SYMTAB`, `DT_RELA`/`DT_RELASZ`/`DT_RELAENT`, and optional PLT relocation entries form core loader metadata. The same specification notes that `DT_NEEDED` order is meaningful, shared-object dependencies form a graph, and no relocation of dynamic-section addresses themselves is used; loader computes memory addresses from the load base.[1]

The Linux `elf(5)` manual specifies that ELF executable/shared-object program headers describe the segments necessary to prepare a program, with `PT_LOAD` carrying file bytes plus zero-filled `p_memsz - p_filesz` tail, `PT_DYNAMIC` carrying dynamic information, and `PT_INTERP` locating the null-terminated interpreter pathname. It states that loadable segments are sorted by virtual address and `p_vaddr`/`p_offset` must be congruent modulo page size for normally aligned segments. It also defines `PF_R`, `PF_W`, and `PF_X` segment flags.[2]

| Design implication | Scope required before compatibility claim |
|---|---|
| ET_DYN loader | Validate ELF64/x86_64 header, bounded program-header table, `PT_LOAD` ranges, overflow and page congruence; map zero-filled BSS tail. |
| Interpreter support | Parse and policy-check one `PT_INTERP`; do not invoke a host linker or claim glibc support. |
| Dynamic metadata | Parse bounded `PT_DYNAMIC`, `DT_STRTAB`, `DT_SYMTAB`, `DT_NEEDED`, `DT_RELA*` and selected relocation subsets. |
| Loader security | Preserve segment permission policy, reject unsupported text relocation/TLS/lazy-binding paths until a W^X-capable mapping policy is proven. |
| Dependency resolution | Requires a real ROMFS pathname/dependency resolver beyond the current fixed release-record server. |

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
[2]: https://man7.org/linux/man-pages/man5/elf.5.html "Linux elf(5) manual page"

The official seL4 x86_64 mapping tutorial states that virtual-memory attributes are architecture-dependent caching attributes and shows page mappings with `seL4_CanRead` or `seL4_ReadWrite` rights plus `seL4_X86_Default_VMAttributes`. It does not establish an execute-disable attribute for the pinned SeLinOS API profile; local generated-header inspection remains controlling for that profile.[3]

[3]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
