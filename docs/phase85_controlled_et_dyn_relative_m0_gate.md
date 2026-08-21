# Phase 85 Gate — controlled `ET_DYN` relative-relocation M0

**Status:** VERIFIED — isolated proof completed; complete regression and publication pending.

The opt-in `x86_64/PC99` QEMU TCG profile validated the fixed `ET_DYN` fixture, applied exactly one checked `R_X86_64_RELATIVE` value through a root-private RW alias, unmapped that alias before target mappings, and reached the second-RX terminal `UD2` witness. ELFRT M1 host coverage passed after relocation metadata summary fields were added.

## Purpose

Phase 85 is the first deliberately narrow transition from the verified static `ET_EXEC` fixture family to **dynamic-ELF loader semantics**. It must load one self-authored, fixed `ELF64`/`x86_64` `ET_DYN` image through a root-owned transaction, materialise its `PT_LOAD` segments under the already verified local x86_64 W^X mapping policy, apply exactly one `R_X86_64_RELATIVE` relocation, and receive one terminal user-exception witness.

> This is a relocation proof, not a dynamic linker. It is intentionally insufficient for glibc, a Debian executable, `PT_INTERP`, `DT_NEEDED`, symbol lookup, `dlopen`, TLS, constructors, `dpkg`, or `apt`.

The phase narrows risk by avoiding external file resolution and symbol scope. The fixture contains no `PT_INTERP`, no `DT_NEEDED`, no `DT_JMPREL`, no TLS program header and no relocation type other than the fixed relative relocation. The root must reject every deviation before mapping any target page.

| Element | Fixed Phase 85 contract | Fail-closed rejection |
|---|---|---|
| ELF form | One self-authored `ELF64` little-endian `ET_DYN`, `EM_X86_64` fixture | Any other type, machine, header range or alignment failure |
| Load layout | Page-aligned RX text, RO+NX metadata, RW+NX data/BSS; no W+X target mapping | Overlap, missing segment, W+X, executable data, writable code or range overflow |
| Dynamic metadata | One bounded `PT_DYNAMIC`; exact `DT_RELA`, `DT_RELASZ`, `DT_RELAENT`; one relocation | Duplicate/missing/malformed entries, relocation-table overflow or unknown dynamic tag required by the fixture |
| Relocation | Exactly one aligned `R_X86_64_RELATIVE` into the owned RW data range | Any other relocation type, relocation outside RW data, nonzero symbol index, addend overflow or write to RX/RO memory |
| Runtime witness | Relocated absolute pointer is read by second-RX code before terminal `UD2` | Missing relocation effect, reply, repair, retry, second resume or alternate terminal fault |
| Resolver policy | No interpreter, dependency, filesystem lookup or host linker use | Any `PT_INTERP`, `DT_NEEDED`, symbol-resolution or host-process shortcut |

## Required implementation boundaries

The parser summary must gain only the metadata necessary to prove the exact `RELA` contract. The root-side transaction must maintain root-private writable aliases solely while materialising segments and applying the checked relocation. Those aliases must be unmapped before all target mappings are installed. The final target maps must be RX for code, RO+NX for metadata and RW+NX for the relocation destination and BSS; no loader frame or source-image note may enter the target address space.

The phase must use a deterministic nonzero PIE load base selected by the fixture protocol. The relocation result must be exactly `load_base + addend`; it must be read from target RW data by the second RX payload. The QEMU proof must bind the terminal fault IP to the second RX page, prove one resume only, and leave the exception unreplied.

## Evidence required for verification

1. An independent verifier binds all fixture, parser, relocation-policy, root-transaction, protocol, CMake, generated-config, image and runtime hashes.
2. Negative host and/or fixture tests reject malformed relocation records before target mapping.
3. An isolated `x86_64/PC99` QEMU TCG profile emits success markers, reaches the exact terminal second-RX `UD2`, and exits only by the bounded harness timeout.
4. ELFRT M1 host-parser evidence is refreshed after each parser interface or behavior change.
5. All profiles rebuild; affected QEMU evidence is replayed; every binding audit reports zero mismatches; the standalone regression suite passes before publication.

## Compatibility sequencing

| Follow-on capability | Earliest prerequisite after Phase 85 |
|---|---|
| `PT_INTERP` handoff | Verified ET_DYN layout and relocation transaction plus controlled initial stack/auxv contract |
| Shared-object dependency graph | Read-only pathname resolver, namespace policy, `DT_NEEDED` ordering and symbol-scope rules |
| glibc dynamic linking | Additional relocation families, TLS, versioning, constructors, lazy/eager binding policy and signal/thread ABI evidence |
| General Linux process ABI | Native `execve`-like lifecycle, credentials, signals, file descriptors, VFS and scheduler/thread semantics |
| Debian binary execution | Reproducible Debian 13.6.0 test corpus passing natively on the preceding services |
| `dpkg` and `apt` | Native filesystem mutation, package database locking, maintainer-script runtime, DNS/TLS/networking, repositories and transaction rollback evidence |

## Explicit non-claims

Phase 85 will not claim a general ELF loader, arbitrary PIE, a general dynamic linker, `PT_INTERP`, shared libraries, glibc, musl dynamic linking, GNU symbol versioning, TLS, constructors, threads, Linux process compatibility, Debian package execution, `dpkg`, `apt`, Linux kernel execution, a Linux VM, a container, chroot or compatibility mode.

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
[2]: https://man7.org/linux/man-pages/man5/elf.5.html "Linux elf(5) manual page"
[3]: https://gitlab.com/x86-psABIs/x86-64-ABI "x86-64 psABI"
