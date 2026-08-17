# SeLinOS ELF runtime M2: execution gate

**Status:** design gate only. No dynamic ELF mapping, relocation, interpreter invocation or execution has been implemented.

ELFRT M1 establishes that SeLinOS can validate metadata for a bounded ELF64/x86_64 `ET_EXEC` or `ET_DYN` byte buffer, including program headers, interpreter declaration, dynamic dependencies, relocation indicators and an initial W^X-oriented policy. This is intentionally separate from a loader. The next stage cannot be accepted merely because a fixture parses.

| Required M2 condition | Current status | Acceptance evidence required |
|---|---|---|
| Map `PT_LOAD` file and zero-filled BSS ranges | Not implemented | Process-local mapping trace plus page-level lifetime tests |
| Preserve non-writable executable segment policy | Blocked for production native code | A proven execute-disable / execute-permission control in the pinned x86_64 mapping API |
| Reject text relocation and W+X segments | Parser policy implemented | Negative fixtures and proof that rejected image is not mapped |
| Resolve `DT_NEEDED` from ROMFS | Not implemented | General pathname/dependency resolver, not current fixed release record |
| Apply x86_64 dynamic relocations | Not implemented | Curated relocation subset, symbol scope rules, overflow tests and no unsupported relocation fall-through |
| Invoke `PT_INTERP` / constructors | Not implemented | Explicit interpreter policy, initial stack/auxv design and code-execution containment evidence |

The current seL4 x86_64 profile exposes cache attributes through its pinned VM mapping interface but no execute-disable control. A direct audit of pinned `libsel4/arch_include/x86/sel4/arch/types.h` lists only `Default`, `WriteBack`, `WriteThrough`, `CacheDisabled`, `Uncacheable` and `WriteCombining` in `seL4_X86_VMAttributes`; no enum value expresses NX or execute permission. `seL4_X86_Page_Map()` accepts generic capability rights plus that cache-only attribute type, and the reviewed x86_64 UnknownSyscall reply frame likewise exposes no architectural execution/TLS state. Official seL4 mapping documentation states that page mapping rights control mapping type while VM attributes are architecture-dependent caching attributes; the x86_64 tutorial’s examples use `seL4_CanRead` or `seL4_ReadWrite` plus `seL4_X86_Default_VMAttributes`.[3] The project has therefore recorded the lack of an auditable NX control in its pinned headers as a blocker for production native Linux module execution. The same limitation prevents an honest claim that a dynamic runtime can safely map arbitrary ELF data writable during load and executable after relocation. Therefore M2 must remain non-executing until either the pinned platform mapping support gains an auditable NX/execute-permission control or the architecture changes to a verified execution-isolation design.

> The ELF ABI describes a dynamic linker as building memory images, applying relocations, resolving dependencies and then transferring control. Parsing `PT_DYNAMIC` or `PT_INTERP` is only preparatory metadata validation, not implementation of that process.[1]

## Required resolution evidence

| Candidate resolution | Required proof before enabling native execution |
|---|---|
| Pinned seL4 upgrade or configuration that exposes execute control | Generated-header/API diff; mapping test proving data pages cannot execute and code pages cannot be writable; page-lifetime and rollback tests. |
| Different verified execution-isolation architecture | Threat model, capability flow, no-W+X invariant proof and independent end-to-end ELF execution evidence. |
| Temporary parser/relocation-only continuation | Negative proof that no target mapping or control transfer occurs; this is the present safe state. |

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
[2]: https://man7.org/linux/man-pages/man5/elf.5.html "Linux elf(5) manual page"
[3]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 Mapping tutorial"
