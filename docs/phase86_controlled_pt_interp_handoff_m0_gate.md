# Phase 86 Gate — controlled `PT_INTERP` handoff M0

**Status:** VERIFIED — isolated x86_64/PC99 QEMU TCG interpreter-entry witness completed; full regression and publication remain pending.

The opt-in profile parser-validated one exact non-host `PT_INTERP` record, materialised a deterministic RW+NX initial stack and bounded auxv record through root-private aliases, unmapped those aliases before target mappings, and transferred exactly once to a self-authored RX interpreter witness. The witness required an aligned stack, `argc=1`, and the fixed `AT_ENTRY` value before terminal `UD2`.

## Purpose

Phase 86 follows the verified Phase 85 `ET_DYN` relative-relocation proof. It will prove that a single self-authored dynamic executable can carry exactly one bounded `PT_INTERP` pathname, that a root-owned transaction can construct a fixed initial stack with a minimal auditable auxiliary-vector record, and that control can be transferred to a **self-authored interpreter witness** rather than directly to the executable entry.

> This phase is a `PT_INTERP` handoff and initial-stack provenance proof. It is not a dynamic linker, glibc, `ld-linux`, shared-object resolver, native Debian executable launcher, Linux ABI implementation, `dpkg`, or `apt`.

| Element | Fixed Phase 86 contract | Rejected before mapping/resume |
|---|---|---|
| Main image | One embedded `ELF64`/`x86_64` `ET_DYN` fixture with one executable `PT_INTERP` declaration | Missing, duplicate, unterminated, oversized or non-policy pathname |
| Interpreter | One separate embedded interpreter witness fixture, no host linker and no external filesystem path lookup | Host process, host dynamic linker, `DT_NEEDED`, symbol lookup or untrusted resolver input |
| Initial stack | Fixed mapped RW+NX page containing bounded `argc`, one `argv` entry, null `envp`, and fixed `AT_PHDR`, `AT_PHENT`, `AT_PHNUM`, `AT_ENTRY`, `AT_PAGESZ`, `AT_NULL` records | Stack-pointer misalignment, pointer outside owned target mappings, duplicate or unknown required auxv record |
| Entry transfer | Target `RIP` is interpreter witness entry; `AT_ENTRY` retains relocated main-image entry | Direct execution of main entry, second resume, exception reply, restart, repair or continuation |
| Witness | Interpreter reads the bounded initial stack and verifies `AT_ENTRY` before terminal `UD2` | Missing stack/auxv validation or failure/alternate terminal fault |

## Safety boundary

The root must construct all writable content through root-private aliases and unmap those aliases before target mappings are installed. Target code pages must remain RX; target metadata and stack must remain NX; no W+X target mapping is permitted. The main image’s `PT_INTERP` record is validated and copied only as parser metadata. It must not be opened through the host, a Linux kernel, a Linux VM, a container, chroot or compatibility mode.

The interpreter witness is intentionally not named `/lib64/ld-linux-x86-64.so.2`, not glibc code, and not a general loader. It will simply prove that transfer starts at the interpreter entry with a correctly bounded stack and auxv representation.

## Evidence required

1. A fixed-fixture validator enforces the one `PT_INTERP` record and all initial-stack constants byte-for-byte.
2. The runtime parser’s host regression remains passing after any `PT_INTERP` parser-summary changes.
3. An isolated x86_64/PC99 QEMU TCG profile proves interpreter entry, stack/auxv witness, terminal `UD2`, one resume and unreplied exception.
4. Independent SHA-bound verification, complete evidence refresh, zero binding mismatches and full standalone regression are required before publication.

## Explicit non-claims

Phase 86 will not claim interpreter search paths, a general ELF loader, glibc or musl, `ld-linux`, shared libraries, `DT_NEEDED`, symbol resolution, relocation beyond the already bounded Phase 85 proof, constructors, TLS, lazy binding, Linux process ABI, `execve`, file-descriptor semantics, Debian package execution, `dpkg`, `apt`, Linux kernel execution, a Linux VM, a container, chroot or compatibility mode.

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
[2]: https://man7.org/linux/man-pages/man5/elf.5.html "Linux elf(5) manual page"
[3]: https://gitlab.com/x86-psABIs/x86-64-ABI "x86-64 psABI"
