# Phase 79: static ELF64 RX load M0 gate

## Status

**Status: VERIFIED — isolated QEMU TCG M0 evidence captured.** The published Phase 78 NXE correction permits the target RX mapping used here. The isolated default-OFF profile validated the exact fixed embedded ELF64 `ET_EXEC` fixture, materialized its `UD2` bytes through the temporary root-private alias, mapped the frame read/execute and non-writable into a fresh target, then observed the terminal entry `UD2` UserException after exactly one resume. It deliberately proves only this single static fixture, not a general executable loader, process launcher, dynamic linker, C runtime, Linux ABI, or Debian compatibility.

## Objective

The M0 transaction must parse one compile-time embedded x86_64 ELF64 `ET_EXEC` byte fixture, apply a strict `PT_LOAD` policy, materialize its two-byte `UD2` text payload through a temporary root-private writable alias, unmap that alias, map the same frame read/execute and non-writable into one fresh suspended target at the validated entry, and perform exactly one resume. The expected terminal witness is the deliberate `UD2` `UserException` at that entry.

> The ELF generic ABI describes `PT_LOAD` as a loadable segment whose file bytes initialize the beginning of the memory image; M0 restricts that general model to a single page and a single RX segment.[1]

| Property | Exact M0 requirement | Rejected form |
|---|---|---|
| ELF identity | ELF64, little-endian, current version, `EM_X86_64`, `ET_EXEC`. | Any other class, data encoding, machine, type, malformed header, or out-of-range table. |
| Program-header table | Exactly one program header, of type `PT_LOAD`; no `PT_INTERP`, `PT_DYNAMIC`, relocation metadata, or additional load segment. | Interpreter, dynamic metadata, relocation, text relocation, extra segment, or any omitted mandatory bound. |
| Segment geometry | `p_offset=0x1000`, `p_vaddr=0x60000000`, `p_align=0x1000`, `p_filesz=p_memsz=2`; file range and alignment must validate. | Any overflow, non-page alignment, nonzero BSS, mismatched address/offset alignment, or target address outside the fixed fixture range. |
| Segment permission | `PF_R|PF_X`, no `PF_W`; no writable-executable segment. | `PF_W`, `PF_W|PF_X`, or absent execute permission. |
| Entry point | `e_entry=0x60000000`, exactly the segment start. | Interior, unmapped, or out-of-policy entry point. |
| Text bytes | The file payload is exactly `0x0f 0x0b` (`UD2`). | Any non-matching fixture byte or digest mismatch. |

## Materialization transaction

The loader M0 must reuse the existing fresh-target construction and NXE-enabled page-mapping primitives rather than create a parallel target model.

| Ordered step | Required action | Fail-closed boundary |
|---|---|---|
| 1 | Parse the embedded fixture using the existing `selinos_elfrt_parse_image()` and reject any nonzero parser or initial-policy result. | No allocation, mapping, or target operation follows a parser/policy rejection. |
| 2 | Validate the M0-only exact fixture geometry, entry and payload digest. | No general ELF acceptance is inferred from one fixture. |
| 3 | Allocate exactly one root-owned 4 KiB frame; map it through one temporary root-private writable alias; copy the two file bytes and zero the remainder. | No target mapping or resume while the root alias exists. The M0 proof makes no separately verified root-alias NX claim. |
| 4 | Unmap and release the root-private writable alias before mapping the same frame into the fresh target at `0x60000000` as readable/executable and non-writable. | Any cleanup, rights, address, or mapping failure tears down and returns false. |
| 5 | Set the fresh target RIP to `0x60000000`; retain its independently proven NX stack; resume exactly once. | No reply, repair, second resume, retry, or post-fault continuation. |
| 6 | Receive only the exact target `UserException` for the deliberate entry `UD2`, then terminate the proof. | Any VMFault, pre-entry fault, wrong badge/IP/RSP/vector, successful continuation, or duplicate event fails closed. |

The x86 `UD2` instruction is intentionally undefined and supplies the terminal non-continuation witness; it is not an ELF program behavior claim.[2]

## Verified evidence

The successful bounded trace contains all three required records: the fixed ELF64 RX `PT_LOAD` validation and materialization marker, the terminal entry-`UD2` no-reply/no-retry marker, and the explicit non-claim marker. The independent verifier SHA-binds the isolated kernel image, root image, transcript, exact fixture, parser, target transaction, CMake selector and this gate. It rejects any unexpected VMFault, bootstrap failure, failure marker, reply, repair, retry or second resume.

## Evidence requirements

A promotable M0 result requires a fresh default-OFF CMake profile, bounded QEMU TCG transcript, SHA-256 binding for the kernel, root image, fixture, parser, loader, mapping control path, protocol and independent verifier. The verifier must reject a writable target mapping, a remaining root writable alias, an interpreter/dynamic/relocation field, a VMFault, reply/repair/retry, or a second resume.

## Non-claims

M0 does **not** prove multiple `PT_LOAD` segments, `ET_DYN`, ASLR, BSS, data segments, ELF section semantics, relocation, TLS, an interpreter, dynamic linking, argv/envp/auxv construction, a C runtime, syscall return, signals, threads, `fork`, credentials, file-descriptor inheritance, VFS execution, package installation, `dpkg`, `apt`, Debian package compatibility, DMA containment, or IOMMU functionality.

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.pheader.html "System V ABI: Program Header"

[2]: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html "Intel® 64 and IA-32 Architectures Software Developer’s Manual"
